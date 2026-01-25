#include "../common/config.h"
#include "../common/logger.h"
#include "../common/protocol.h"
#include "../common/sockets.h"
#include "../common/utils.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

PlaneConfig config;
uint32_t current_fuel = 0;
int current_airport_idx = 0;

int connect_to_airport(const char *ip, uint16_t port, int *sock_fd) {
  *sock_fd = connect_to(ip, port);
  if (*sock_fd < 0) {
    log_message(LOG_ERROR, "Failed to connect to airport %s:%d", ip, port);
    return -1;
  }

  char buffer[1024];

  // Receive airport handshake
  int received = recv(*sock_fd, buffer, sizeof(buffer) - 1, 0);
  if (received <= 0) {
    log_message(LOG_ERROR, "Failed to receive airport handshake");
    close(*sock_fd);
    return -1;
  }
  buffer[received] = '\0';
  log_message(LOG_INFO, "Airport says: %s", buffer);

  // Check if busy
  if (strstr(buffer, "busy") != NULL) {
    log_message(LOG_WARNING, "Airport is busy");
    close(*sock_fd);
    return -2;
  }

  // Send plane handshake
  snprintf(buffer, sizeof(buffer),
           "AVN: Avion id: %s iniciando comunicacion\\r\\n", config.id);
  if (send_all(*sock_fd, buffer, strlen(buffer)) < 0) {
    log_message(LOG_ERROR, "Failed to send handshake");
    close(*sock_fd);
    return -1;
  }

  // Receive acceptance
  received = recv(*sock_fd, buffer, sizeof(buffer) - 1, 0);
  if (received <= 0) {
    log_message(LOG_ERROR, "Failed to receive acceptance");
    close(*sock_fd);
    return -1;
  }
  buffer[received] = '\0';
  log_message(LOG_INFO, "Airport response: %s", buffer);

  if (strstr(buffer, "aceptada") == NULL) {
    log_message(LOG_ERROR, "Airport did not accept connection");
    close(*sock_fd);
    return -1;
  }

  log_message(LOG_EVENT, "Connected to airport successfully");
  return 0;
}

int request_fuel(int airport_fd) {
  // Request fuel station info
  send_message(airport_fd, MSG_PEDIDO_COMBUSTIBLE, NULL, 0);
  log_message(LOG_INFO, "Requested fuel station info");

  uint16_t msg_type;
  struct Payload_DatosCombustible fuel_data;
  int len = recv_message(airport_fd, &msg_type, &fuel_data, sizeof(fuel_data));

  if (len < 0 || msg_type != MSG_DATOS_COMBUSTIBLE) {
    log_message(LOG_ERROR, "Failed to get fuel station info");
    return -1;
  }

  // Convert IP
  struct in_addr addr;
  addr.s_addr = fuel_data.ip;
  char fuel_ip[16];
  strcpy(fuel_ip, inet_ntoa(addr));

  log_message(LOG_INFO, "Fuel station at %s:%d", fuel_ip, fuel_data.port);

  // Connect to fuel station
  int fuel_fd = connect_to(fuel_ip, fuel_data.port);
  if (fuel_fd < 0) {
    log_message(LOG_ERROR, "Failed to connect to fuel station");
    return -1;
  }

  char buffer[1024];

  // Fuel handshake - receive
  int received = recv(fuel_fd, buffer, sizeof(buffer) - 1, 0);
  if (received <= 0) {
    close(fuel_fd);
    return -1;
  }
  buffer[received] = '\0';
  log_message(LOG_INFO, "Fuel station says: %s", buffer);

  // Fuel handshake - send
  snprintf(buffer, sizeof(buffer),
           "AVN: Avion id: %s solicita combustible\\r\\n", config.id);
  send_all(fuel_fd, buffer, strlen(buffer));

  // Wait for confirmation
  received = recv(fuel_fd, buffer, sizeof(buffer) - 1, 0);
  if (received <= 0) {
    close(fuel_fd);
    return -1;
  }
  buffer[received] = '\0';
  log_message(LOG_INFO, "Fuel station response: %s", buffer);

  // Send fuel request
  uint32_t gallons_needed = 3000; // Simplified
  struct Payload_PedidoCombustible req;
  req.galones = gallons_needed;

  send_message(fuel_fd, MSG_FUEL_REQUEST, &req, sizeof(req));
  log_message(LOG_EVENT, "Requested %u gallons of fuel", gallons_needed);

  // Wait for fuel finished
  uint8_t dummy[10];
  len = recv_message(fuel_fd, &msg_type, dummy, sizeof(dummy));
  if (len < 0 || msg_type != MSG_FUEL_FINISHED) {
    log_message(LOG_ERROR, "Fuel loading failed");
    close(fuel_fd);
    return -1;
  }

  log_message(LOG_EVENT, "Fuel loading complete");
  current_fuel += gallons_needed;

  close(fuel_fd);
  return 0;
}

int main(int argc, char *argv[]) {
  const char *config_file = (argc > 1) ? argv[1] : "plane.conf";

  if (load_plane_config(config_file, &config) < 0) {
    fprintf(stderr, "Failed to load plane config\\n");
    return 1;
  }

  // Initialize logger
  char log_file[64];
  snprintf(log_file, sizeof(log_file), "plane_%s.log", config.id);
  log_init(log_file, config.id);

  log_message(LOG_INFO, "Plane %s starting", config.id);
  log_message(LOG_INFO, "Journey: %d airports, Trip type: %c",
              config.num_airports, config.trip_type);

  current_fuel = config.initial_fuel;
  printf("Plane %s starting journey\\n", config.id);

  // Travel through journey
  for (current_airport_idx = 0; current_airport_idx < config.num_airports;
       current_airport_idx++) {
    uint32_t airport_id = config.airport_ids[current_airport_idx];
    log_message(LOG_EVENT, "Flying to airport %u (stop %d/%d)", airport_id,
                current_airport_idx + 1, config.num_airports);

    // Connect to airport (using first airport config for now - simplified)
    int airport_fd;
    if (connect_to_airport(config.first_airport_ip, config.first_airport_port,
                           &airport_fd) < 0) {
      log_message(LOG_ERROR, "Failed to connect to airport");
      return 1;
    }

    // Request terminal entry
    send_message(airport_fd, MSG_INGRESO_TERMINAL, NULL, 0);
    log_message(LOG_INFO, "Requested terminal entry");

    uint16_t msg_type;
    uint8_t dummy[10];
    recv_message(airport_fd, &msg_type, dummy, sizeof(dummy));
    if (msg_type == MSG_OK_INGRESO) {
      log_message(LOG_EVENT, "Terminal entry granted");
    }

    // Request runway (for landing)
    send_message(airport_fd, MSG_PEDIDO_PISTA, NULL, 0);
    log_message(LOG_INFO, "Requested runway for landing");

    recv_message(airport_fd, &msg_type, dummy, sizeof(dummy));
    if (msg_type == MSG_PISTA_OTORGADA) {
      log_message(LOG_EVENT, "Runway granted, landing");
      sleep(1); // Simulate landing
    }

    // Refuel - ALWAYS for planes in transit (Aclaraciones line 42)
    // "Los aviones en transito, aterrizan, realizan recarga de combustible..."
    if (current_airport_idx < config.num_airports - 1) {
      log_message(LOG_INFO, "In transit, refueling (current: %u gallons)",
                  current_fuel);
      request_fuel(airport_fd);
    } else {
      // Last airport - only refuel if low
      if (current_fuel < 2000) {
        log_message(LOG_INFO, "Low fuel (%u), requesting refuel", current_fuel);
        request_fuel(airport_fd);
      }
    }

    // Request takeoff if not last airport
    if (current_airport_idx < config.num_airports - 1) {
      send_message(airport_fd, MSG_PEDIDO_PISTA, NULL, 0);
      log_message(LOG_INFO, "Requested runway for takeoff");

      recv_message(airport_fd, &msg_type, dummy, sizeof(dummy));
      if (msg_type == MSG_PISTA_OTORGADA) {
        log_message(LOG_EVENT, "Runway granted, taking off");
        sleep(1);            // Simulate takeoff
        current_fuel -= 500; // Consume fuel
      }
    }

    // Notify leaving airspace
    send_message(airport_fd, MSG_ABANDONO_ESPACIO, NULL, 0);
    log_message(LOG_EVENT, "Left airspace");

    close(airport_fd);
  }

  log_message(LOG_EVENT, "Journey complete!");
  printf("Plane %s completed journey!\\n", config.id);

  log_close();
  return 0;
}
