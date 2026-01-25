#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

// Generate unique message ID
uint32_t generate_msg_id();

// Generate unique 16-byte beacon message ID
void generate_beacon_msg_id(uint8_t *id);

// Helper to send full message with header
int send_message(int sockfd, uint16_t tipo, const void *payload,
                 uint32_t payload_size);

// Helper to receive message with header
int recv_message(int sockfd, uint16_t *tipo, void *payload,
                 uint32_t max_payload_size);

#endif // UTILS_H
