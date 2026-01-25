#include "../common/config.h"
#include "../common/logger.h"
#include "../common/protocol.h"
#include "../common/sockets.h"
#include "../common/utils.h"
#include "runway.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// Global configuration
AirportConfig config;
RunwayManager *runway_mgr = NULL;
int terminal_count = 0;
pthread_mutex_t terminal_mutex = PTHREAD_MUTEX_INITIALIZER;

// Export for runway.c
int landing_time_ms;
int takeoff_time_ms;

void *handle_client(void *arg) {
  int client_fd = *(int *)arg;
  free(arg);

  char buffer[4096];
  char plane_id[7] = "";

  // 1. Send handshake: AER: AeroPuerto Listo. Id: XXXX Protocolo v1.0\r\n
  snprintf(buffer, sizeof(buffer),
           "AER: AeroPuerto Listo. Id: %s Protocolo v1.0\r\n", config.id);
  if (send_all(client_fd, buffer, strlen(buffer)) < 0) {
    log_message(LOG_ERROR, "Failed to send handshake");
    close(client_fd);
    return NULL;
  }
  log_message(LOG_INFO, "Sent handshake to client");

  // 2. Receive: AVN: Avion id: YYYYYY iniciando comunicacion\r\n
  memset(buffer, 0, sizeof(buffer));
  int received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
  if (received <= 0) {
    log_message(LOG_ERROR, "Failed to receive plane handshake");
    close(client_fd);
    return NULL;
  }
  buffer[received] = '\0';

  // Parse plane ID
  if (sscanf(buffer, "AVN: Avion id: %6s", plane_id) != 1) {
    log_message(LOG_ERROR, "Failed to parse plane ID");
    close(client_fd);
    return NULL;
  }
  log_message(LOG_INFO, "Received handshake from plane %s", plane_id);

  // 3. Check airport status and send response
  if (config.status == STATUS_CERRADO) {
    const char *busy_msg = "AER: busy\r\n";
    send_all(client_fd, busy_msg, strlen(busy_msg));
    log_message(LOG_WARNING, "Airport closed, rejected plane %s", plane_id);
    close(client_fd);
    return NULL;
  }

  const char *accept_msg = "AER: Comunicacion aceptada\r\n";
  if (send_all(client_fd, accept_msg, strlen(accept_msg)) < 0) {
    log_message(LOG_ERROR, "Failed to send acceptance to plane %s", plane_id);
    close(client_fd);
    return NULL;
  }
  log_message(LOG_EVENT, "Accepted connection from plane %s", plane_id);

  // Main message handling loop
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
    case MSG_INGRESO_TERMINAL: {
      log_message(LOG_EVENT, "Plane %s requesting terminal entry", plane_id);

      pthread_mutex_lock(&terminal_mutex);
      if (terminal_count >= config.terminal_quota) {
        pthread_mutex_unlock(&terminal_mutex);
        log_message(LOG_WARNING, "Terminal full, plane %s waiting", plane_id);
        // Block until space available (simplified - should use cond var)
        sleep(1);
        pthread_mutex_lock(&terminal_mutex);
      }
      terminal_count++;
      pthread_mutex_unlock(&terminal_mutex);

      send_message(client_fd, MSG_OK_INGRESO, NULL, 0);
      log_message(LOG_EVENT, "Granted terminal entry to plane %s (count: %d)",
                  plane_id, terminal_count);
      break;
    }

    case MSG_PEDIDO_PISTA:
    case MSG_PEDIDO_PISTA_EMERGENCIA: {
      int is_emergency = (msg_type == MSG_PEDIDO_PISTA_EMERGENCIA);
      // Determine if landing or takeoff based on terminal status
      // Simplified: assume planes in terminal are taking off
      int is_landing = (terminal_count == 0);

      log_message(LOG_EVENT, "Plane %s requesting runway (%s, %s)", plane_id,
                  is_landing ? "landing" : "takeoff",
                  is_emergency ? "EMERGENCY" : "normal");

      int runway = runway_enqueue(runway_mgr, client_fd, plane_id, is_landing,
                                  is_emergency);
      if (runway < 0) {
        log_message(LOG_ERROR, "Failed to assign runway to plane %s", plane_id);
        // Could send MSG_PISTA_CERRADA here
      }
      // MSG_PISTA_OTORGADA will be sent by runway thread
      break;
    }

    case MSG_PEDIDO_COMBUSTIBLE: {
      log_message(LOG_EVENT, "Plane %s requesting fuel station info", plane_id);

      // TODO: Get fuel station info from config
      struct Payload_DatosCombustible fuel_data;
      fuel_data.ip = inet_addr("127.0.0.1");
      fuel_data.port = 8091;

      send_message(client_fd, MSG_DATOS_COMBUSTIBLE, &fuel_data,
                   sizeof(fuel_data));
      log_message(LOG_INFO, "Sent fuel station info to plane %s", plane_id);
      break;
    }

    case MSG_PEDIDO_RUTA: {
      struct Payload_PedidoRuta *req = (struct Payload_PedidoRuta *)payload;
      log_message(LOG_EVENT, "Plane %s requesting route to airport %u",
                  plane_id, req->id_aeropuerto);

      // TODO: Query beacon for route
      // For now, send unknown airport
      send_message(client_fd, MSG_AEROPUERTO_DESC, NULL, 0);
      log_message(LOG_WARNING, "Route discovery not implemented, sent unknown");
      break;
    }

    case MSG_INGRESO_ESPACIO: {
      log_message(LOG_EVENT, "Plane %s entering airspace", plane_id);
      // Parse ingreso espacio payload if needed
      send_message(client_fd, MSG_OK, NULL, 0);
      break;
    }

    case MSG_ABANDONO_ESPACIO: {
      log_message(LOG_EVENT, "Plane %s leaving airspace", plane_id);

      pthread_mutex_lock(&terminal_mutex);
      if (terminal_count > 0) {
        terminal_count--;
      }
      pthread_mutex_unlock(&terminal_mutex);

      send_message(client_fd, MSG_OK, NULL, 0);
      log_message(LOG_INFO, "Plane %s departed, terminal count: %d", plane_id,
                  terminal_count);
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

int main(int argc, char *argv[]) {
  // Load configuration
  const char *config_file = (argc > 1) ? argv[1] : "airport.conf";

  if (load_airport_config(config_file, &config) < 0) {
    fprintf(stderr, "Failed to load config from %s\n", config_file);
    return 1;
  }

  // Initialize logger
  char log_filename[64];
  snprintf(log_filename, sizeof(log_filename), "airport_%s.log", config.id);
  log_init(log_filename, config.id);

  log_message(LOG_INFO, "Airport %s starting on port %d", config.id,
              config.port);
  log_message(LOG_INFO, "Runways: %d, Terminal quota: %d", config.num_runways,
              config.terminal_quota);

  // Set global timing
  landing_time_ms = config.landing_time_ms;
  takeoff_time_ms = config.takeoff_time_ms;

  // Initialize runway manager
  runway_mgr = runway_manager_init(config.num_runways, config.runway_types);
  if (!runway_mgr) {
    log_message(LOG_ERROR, "Failed to initialize runway manager");
    return 1;
  }

  // Start runway threads
  pthread_t runway_threads[config.num_runways];
  for (int i = 0; i < config.num_runways; i++) {
    if (pthread_create(&runway_threads[i], NULL, runway_process_thread,
                       &runway_mgr->runways[i]) != 0) {
      log_message(LOG_ERROR, "Failed to create runway thread %d", i);
      return 1;
    }
    log_message(LOG_INFO, "Runway %d thread started (type: %d)", i,
                config.runway_types[i]);
  }

  // Bind and listen
  int server_fd = server_bind_listen(config.port);
  if (server_fd < 0) {
    log_message(LOG_ERROR, "Failed to bind to port %d", config.port);
    return 1;
  }
  log_message(LOG_INFO, "Airport listening on port %d", config.port);

  printf("Airport %s running on port %d\n", config.id, config.port);
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

    // Handle client in new thread
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
  runway_manager_free(runway_mgr);
  free_airport_config(&config);
  log_close();

  return 0;
}
