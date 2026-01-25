#include "../common/protocol.h"
#include "../common/sockets.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define AIRPORT_ID "A123"
#define PORT 8090

int main() {
  printf("Airport Process Starting...\n");
  printf("Header size: %lu bytes\n", sizeof(struct Header));

  int server_fd = server_bind_listen(PORT);
  if (server_fd < 0) {
    fprintf(stderr, "Failed to bind server\n");
    return 1;
  }
  printf("Airport listening on port %d\n", PORT);

  struct sockaddr_in client_addr;
  socklen_t addr_len = sizeof(client_addr);
  int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
  if (client_fd < 0) {
    perror("accept");
    close(server_fd);
    return 1;
  }
  printf("Client connected\n");

  // 1. Send: AER: AeroPuerto Listo. Id: XXXX Protocolo v1.0\r\n
  char msg_initial[100];
  sprintf(msg_initial, "AER: AeroPuerto Listo. Id: %s Protocolo v1.0\r\n",
          AIRPORT_ID);
  if (send_all(client_fd, msg_initial, strlen(msg_initial)) < 0) {
    fprintf(stderr, "Failed to send initial message\n");
    close(client_fd);
    close(server_fd);
    return 1;
  }
  printf("Sent: %s", msg_initial);

  // 2. Receive: AVN: Avion id: YYYYYY inciando comunicacion\r\n
  char buffer[1024];
  memset(buffer, 0, sizeof(buffer));
  // Not robust framing, just reading enough for handshake for now as per simple
  // verification
  int received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
  if (received <= 0) {
    fprintf(stderr, "Failed to receive handshake\n");
    close(client_fd);
    close(server_fd);
    return 1;
  }
  buffer[received] = '\0';
  printf("Received: %s", buffer);

  // 3. Send: AER: Comunicacion aceptada\r\n
  const char *msg_ok = "AER: Comunicacion aceptada\r\n";
  if (send_all(client_fd, msg_ok, strlen(msg_ok)) < 0) {
    fprintf(stderr, "Failed to send acceptance\n");
    close(client_fd);
    close(server_fd);
    return 1;
  }
  printf("Sent: %s", msg_ok);

  printf("Handshake completed successfully.\n");

  close(client_fd);
  close(server_fd);
  return 0;
}
