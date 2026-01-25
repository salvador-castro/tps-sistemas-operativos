#include "../common/protocol.h"
#include "../common/sockets.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define FUEL_PORT 8091

int main() {
  printf("Fuel Process Starting...\n");

  int server_fd = server_bind_listen(FUEL_PORT);
  if (server_fd < 0) {
    fprintf(stderr, "Fuel: Failed to bind server\n");
    return 1;
  }
  printf("Fuel Station listening on port %d\n", FUEL_PORT);

  struct sockaddr_in client_addr;
  socklen_t addr_len = sizeof(client_addr);
  int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
  if (client_fd < 0) {
    perror("accept");
    close(server_fd);
    return 1;
  }
  printf("Fuel: Client connected\n");

  // 1. Send: COM: Estación de recarga Lista. Protocolo v1.0\r\n
  const char *msg_initial =
      "COM: Estacion de recarga Lista. Protocolo v1.0\r\n";
  if (send_all(client_fd, msg_initial, strlen(msg_initial)) < 0) {
    fprintf(stderr, "Fuel: Failed to send initial message\n");
    close(client_fd);
    close(server_fd);
    return 1;
  }
  printf("Sent: %s", msg_initial);

  // 2. Receive: AVN: Avion id: YYYYYY solicita combustible\r\n
  char buffer[1024];
  memset(buffer, 0, sizeof(buffer));
  int received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
  if (received <= 0) {
    fprintf(stderr, "Fuel: Failed to receive handshake\n");
    close(client_fd);
    close(server_fd);
    return 1;
  }
  buffer[received] = '\0';
  printf("Received: %s", buffer);

  // 3. Send: COM: Esperando pedido\r\n
  const char *msg_ok = "COM: Esperando pedido\r\n";
  if (send_all(client_fd, msg_ok, strlen(msg_ok)) < 0) {
    fprintf(stderr, "Fuel: Failed to send waiting message\n");
    close(client_fd);
    close(server_fd);
    return 1;
  }
  printf("Sent: %s", msg_ok);

  printf("Fuel Handshake completed successfully.\n");

  close(client_fd);
  close(server_fd);
  return 0;
}
