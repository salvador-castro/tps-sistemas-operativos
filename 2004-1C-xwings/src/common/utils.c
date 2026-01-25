#include "utils.h"
#include "protocol.h"
#include "sockets.h"
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>

static uint32_t msg_counter = 0;

uint32_t generate_msg_id() {
  if (msg_counter == 0) {
    srand(time(NULL));
    msg_counter = rand();
  }
  return msg_counter++;
}

void generate_beacon_msg_id(uint8_t *id) {
  static int initialized = 0;
  if (!initialized) {
    srand(time(NULL));
    initialized = 1;
  }
  for (int i = 0; i < 16; i++) {
    id[i] = rand() % 256;
  }
}

int send_message(int sockfd, uint16_t tipo, const void *payload,
                 uint32_t payload_size) {
  struct Header hdr;
  hdr.id_msg = generate_msg_id();
  hdr.tipo = tipo;
  hdr.largo_msg = payload_size;

  // Send header
  if (send_all(sockfd, &hdr, sizeof(hdr)) < 0) {
    return -1;
  }

  // Send payload if present
  if (payload_size > 0 && payload != NULL) {
    if (send_all(sockfd, payload, payload_size) < 0) {
      return -1;
    }
  }

  return 0;
}

int recv_message(int sockfd, uint16_t *tipo, void *payload,
                 uint32_t max_payload_size) {
  struct Header hdr;

  // Receive header
  int received = recv(sockfd, &hdr, sizeof(hdr), 0);
  if (received != sizeof(hdr)) {
    return -1;
  }

  *tipo = hdr.tipo;

  // Receive payload if present
  if (hdr.largo_msg > 0) {
    if (hdr.largo_msg > max_payload_size) {
      return -1; // Payload too large
    }
    received = recv(sockfd, payload, hdr.largo_msg, 0);
    if (received != (int)hdr.largo_msg) {
      return -1;
    }
  }

  return hdr.largo_msg;
}
