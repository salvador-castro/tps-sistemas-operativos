#ifndef SOCKETS_H
#define SOCKETS_H

#include <stddef.h>
#include <stdint.h>

// Creates a server socket bound to the specific port.
// Returns the socket file descriptor, or -1 on error.
int server_bind_listen(int port);

// Connects to a server at ip:port.
// Returns the socket file descriptor, or -1 on error.
// ip should be string representation (e.g., "127.0.0.1").
int connect_to(const char *ip, int port);

// Sends exactly len bytes from buf to sockfd.
// Returns 0 on success, -1 on error.
int send_all(int sockfd, const void *buf, size_t len);

// Receives exactly len bytes into buf from sockfd.
// Returns 0 on success, -1 on error or connection closed.
int recv_exact(int sockfd, void *buf, size_t len);

#endif // SOCKETS_H
