#include "../common/protocol.h"
#include "../common/sockets.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PLANE_ID "P99999"
#define AIRPORT_IP "127.0.0.1"
#define AIRPORT_PORT 8090

int main() {
  printf("Plane Process Starting...\n");

  int sock_fd = connect_to(AIRPORT_IP, AIRPORT_PORT);
  if (sock_fd < 0) {
    fprintf(stderr, "Failed to connect to airport\n");
    return 1;
  }
  printf("Connected to Airport at %s:%d\n", AIRPORT_IP, AIRPORT_PORT);

  // 1. Receive: AER: AeroPuerto Listo...
  char buffer[1024];
  memset(buffer, 0, sizeof(buffer));
  int received = recv(sock_fd, buffer, sizeof(buffer) - 1, 0);
  if (received <= 0) {
    fprintf(stderr, "Failed to receive initial message\n");
    close(sock_fd);
    return 1;
  }
  buffer[received] = '\0';
  printf("Received: %s", buffer);

  // 2. Send: AVN: Avion id: YYYYYY iniciando comunicacion\r\n
  char msg_response[100];
  sprintf(msg_response, "AVN: Avion id: %s iniciando comunicacion\r\n",
          PLANE_ID);
  if (send_all(sock_fd, msg_response, strlen(msg_response)) < 0) {
    fprintf(stderr, "Failed to send response\n");
    close(sock_fd);
    return 1;
  }
  printf("Sent: %s", msg_response);

  // 3. Receive: AER: Comunicacion aceptada\r\n or AER: busy\r\n
  memset(buffer, 0, sizeof(buffer));
  received = recv(sock_fd, buffer, sizeof(buffer) - 1, 0);
  if (received <= 0) {
    fprintf(stderr, "Failed to receive acceptance\n");
    close(sock_fd);
    return 1;
  }
  buffer[received] = '\0';
  printf("Received: %s", buffer);

  // Check if airport is busy
  if (strstr(buffer, "busy") != NULL) {
    fprintf(stderr, "Airport is busy, cannot establish communication\n");
    close(sock_fd);
    return 1;
  }

  printf("Handshake with Airport completed successfully.\n");
  // Keep socket open - connection must remain until plane leaves airspace
  printf("Connection with Airport remains open (protocol requirement)\n");

  // --- Fuel Station Communication ---
  printf("\n--- Connecting to Fuel Station ---\n");

#define FUEL_IP "127.0.0.1"
#define FUEL_PORT 8091

  sock_fd = connect_to(FUEL_IP, FUEL_PORT);
  if (sock_fd < 0) {
    fprintf(stderr, "Failed to connect to Fuel Station\n");
    return 1;
  }
  printf("Connected to Fuel Station at %s:%d\n", FUEL_IP, FUEL_PORT);

  // 1. Receive: COM: Estación de recarga Lista...
  memset(buffer, 0, sizeof(buffer));
  received = recv(sock_fd, buffer, sizeof(buffer) - 1, 0);
  if (received <= 0) {
    fprintf(stderr, "Failed to receive Fuel initial message\n");
    close(sock_fd);
    return 1;
  }
  buffer[received] = '\0';
  printf("Received: %s", buffer);

  // 2. Send: AVN: Avion id: YYYYYY solicita combustible\r\n
  sprintf(msg_response, "AVN: Avion id: %s solicita combustible\r\n", PLANE_ID);
  if (send_all(sock_fd, msg_response, strlen(msg_response)) < 0) {
    fprintf(stderr, "Failed to send Fuel response\n");
    close(sock_fd);
    return 1;
  }
  printf("Sent: %s", msg_response);

  // 3. Receive: COM: Esperando pedido\r\n
  memset(buffer, 0, sizeof(buffer));
  received = recv(sock_fd, buffer, sizeof(buffer) - 1, 0);
  if (received <= 0) {
    fprintf(stderr, "Failed to receive Fuel confirmation\n");
    close(sock_fd);
    return 1;
  }
  buffer[received] = '\0';
  printf("Received: %s", buffer);

  printf("Handshake with Fuel Station completed successfully.\n");

  // Clean up
  close(sock_fd);

  printf("\n=== All handshakes completed ===\n");
  return 0;
}
