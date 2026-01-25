#include "../common/config.h"
#include "../common/logger.h"
#include "../common/protocol.h"
#include "../common/sockets.h"
#include "../common/utils.h"
#include "scheduler.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

FuelConfig config;
FuelManager *fuel_mgr = NULL;

void *handle_client(void *arg) {
  int client_fd = *(int *)arg;
  free(arg);

  char buffer[1024];
  char plane_id[7] = "";

  // 1. Send handshake
  const char *msg_init = "COM: Estacion de recarga Lista. Protocolo v1.0\r\n";
  if (send_all(client_fd, msg_init, strlen(msg_init)) < 0) {
    log_message(LOG_ERROR, "Failed to send handshake");
    close(client_fd);
    return NULL;
  }

  // 2. Receive plane handshake
  memset(buffer, 0, sizeof(buffer));
  int received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
  if (received <= 0) {
    log_message(LOG_ERROR, "Failed to receive plane handshake");
    close(client_fd);
    return NULL;
  }
  buffer[received] = '\0';

  if (sscanf(buffer, "AVN: Avion id: %6s solicita combustible", plane_id) !=
      1) {
    log_message(LOG_ERROR, "Failed to parse plane ID from: %s", buffer);
    close(client_fd);
    return NULL;
  }

  log_message(LOG_INFO, "Received fuel request handshake from plane %s",
              plane_id);

  // 3. Send waiting message
  const char *msg_wait = "COM: Esperando pedido\r\n";
  if (send_all(client_fd, msg_wait, strlen(msg_wait)) < 0) {
    log_message(LOG_ERROR, "Failed to send waiting message");
    close(client_fd);
    return NULL;
  }

  // Handle protocol messages
  while (1) {
    uint16_t msg_type;
    uint8_t payload[4096];

    int payload_len =
        recv_message(client_fd, &msg_type, payload, sizeof(payload));
    if (payload_len < 0) {
      log_message(LOG_INFO, "Plane %s disconnected", plane_id);
      break;
    }

    switch (msg_type) {
    case MSG_FUEL_REQUEST: {
      struct Payload_PedidoCombustible *req =
          (struct Payload_PedidoCombustible *)payload;
      log_message(LOG_EVENT, "Plane %s requesting %u gallons", plane_id,
                  req->galones);

      int tank = fuel_enqueue(fuel_mgr, client_fd, plane_id, req->galones);
      if (tank < 0) {
        log_message(LOG_ERROR, "Failed to enqueue fuel request for plane %s",
                    plane_id);
      }
      // MSG_FUEL_FINISHED will be sent by tank thread when complete
      // Keep connection open until then
      break;
    }

    default:
      log_message(LOG_WARNING, "Unknown message type 0x%04X from plane %s",
                  msg_type, plane_id);
      break;
    }
  }

  close(client_fd);
  return NULL;
}

void signal_handler(int sig) {
  (void)sig;
  printf("\nReceived signal, shutting down...\n");
  log_message(LOG_INFO, "Fuel station shutting down");
  fuel_manager_free(fuel_mgr);
  log_close();
  exit(0);
}

int main(int argc, char *argv[]) {
  // Load configuration
  const char *config_file = (argc > 1) ? argv[1] : "fuel.conf";

  if (load_fuel_config(config_file, &config) < 0) {
    fprintf(stderr, "Failed to load config from %s\n", config_file);
    return 1;
  }

  // Initialize logger
  log_init("fuel_station.log", "FUEL");

  log_message(LOG_INFO, "Fuel station starting on port %d", config.port);
  log_message(LOG_INFO, "Tanks: %d, Scheduler: %s, Quantum: %d ms",
              config.num_tanks, config.scheduler, config.quantum_ms);

  // Parse scheduler type
  SchedulerType sched = FUEL_SCHED_RR;
  if (strcmp(config.scheduler, "FIFO") == 0) {
    sched = FUEL_SCHED_FIFO;
  } else if (strcmp(config.scheduler, "SRT") == 0) {
    sched = FUEL_SCHED_SRT;
  }

  // Initialize fuel manager
  fuel_mgr = fuel_manager_init(config.num_tanks, sched, config.quantum_ms,
                               config.gallons_per_quantum);
  if (!fuel_mgr) {
    log_message(LOG_ERROR, "Failed to initialize fuel manager");
    return 1;
  }

  // Start tank threads
  pthread_t tank_threads[config.num_tanks];
  for (int i = 0; i < config.num_tanks; i++) {
    TankThreadArg *arg = malloc(sizeof(TankThreadArg));
    arg->tank = &fuel_mgr->tanks[i];
    arg->manager = fuel_mgr;

    if (pthread_create(&tank_threads[i], NULL, fuel_tank_thread, arg) != 0) {
      log_message(LOG_ERROR, "Failed to create tank thread %d", i);
      return 1;
    }
    log_message(LOG_INFO, "Fuel tank %d thread started", i);
  }

  // Setup signal handler
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  // Bind and listen
  int server_fd = server_bind_listen(config.port);
  if (server_fd < 0) {
    log_message(LOG_ERROR, "Failed to bind to port %d", config.port);
    return 1;
  }
  log_message(LOG_INFO, "Fuel station listening on port %d", config.port);

  printf("Fuel Station running on port %d with %d tanks (%s scheduler)\n",
         config.port, config.num_tanks, config.scheduler);
  printf("Press Ctrl+C to stop\n");

  // Accept client connections
  while (1) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_fd =
        accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);

    if (client_fd < 0) {
      log_message(LOG_ERROR, "Failed to accept client");
      continue;
    }

    log_message(LOG_INFO, "New client connected from %s:%d",
                inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

    pthread_t client_thread;
    int *fd_ptr = malloc(sizeof(int));
    *fd_ptr = client_fd;
    if (pthread_create(&client_thread, NULL, handle_client, fd_ptr) != 0) {
      log_message(LOG_ERROR, "Failed to create client thread");
      close(client_fd);
      free(fd_ptr);
    } else {
      pthread_detach(client_thread);
    }
  }

  // Cleanup
  close(server_fd);
  fuel_manager_free(fuel_mgr);
  log_close();

  return 0;
}
