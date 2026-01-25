#include "sockets.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int server_bind_listen(int port) {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("socket");
    return -1;
  }

  int opt = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("setsockopt");
    close(sockfd);
    return -1;
  }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    close(sockfd);
    return -1;
  }

  if (listen(sockfd, 10) < 0) {
    perror("listen");
    close(sockfd);
    return -1;
  }

  return sockfd;
}

int connect_to(const char *ip, int port) {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    perror("socket");
    return -1;
  }

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
    perror("inet_pton");
    close(sockfd);
    return -1;
  }

  if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("connect");
    close(sockfd);
    return -1;
  }

  return sockfd;
}

int send_all(int sockfd, const void *buf, size_t len) {
  size_t total_sent = 0;
  const char *ptr = (const char *)buf;

  while (total_sent < len) {
    ssize_t sent = send(sockfd, ptr + total_sent, len - total_sent, 0);
    if (sent <= 0) {
      perror("send");
      return -1;
    }
    total_sent += sent;
  }
  return 0;
}

int recv_exact(int sockfd, void *buf, size_t len) {
  size_t total_received = 0;
  char *ptr = (char *)buf;

  while (total_received < len) {
    ssize_t received =
        recv(sockfd, ptr + total_received, len - total_received, 0);
    if (received < 0) {
      perror("recv");
      return -1;
    }
    if (received == 0) {
      // Connection closed
      return -1;
    }
    total_received += received;
  }
  return 0;
}
