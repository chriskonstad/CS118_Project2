#ifndef LIB_PACKET_H
#define LIB_PACKET_H

#include <assert.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * The Packet struct represents a packet's header and payload data.
 * The helper functions can be used to send and receive packets as byte arrays.
 */

const int MAX_PACKET_SIZE = 1024;
const int PACKET_HEADER_LENGTH = 8;  // two 4 byte values, adjust as needed

typedef struct Packet {
  uint32_t seq;   // seq number
  uint32_t ack;   // ack number
  uint8_t *data;  // byte array received from socket, excluding header
  size_t length;  // length of data section
} Packet;

/**
 * Read a byte array (a serialized packet) into a packet.
 * The packet must later be freed with freePacket, and the passed in
 * data array is freed.
 * The passed in data array is freed.
 */
void parsePacket(const uint8_t *const data, size_t length, Packet *packet) {
  // Parse data
  const uint32_t *dataAs32Bit = (uint32_t *)data;
  packet->seq = ntohl(dataAs32Bit[0]);
  packet->ack = ntohl(dataAs32Bit[1]);
  packet->length = length - PACKET_HEADER_LENGTH;

  // Have the packet have its own copy of the data
  packet->data = (uint8_t *)malloc(packet->length * sizeof(uint8_t));
  assert(packet->data);
  memcpy(packet->data, (uint8_t *)&dataAs32Bit[2], packet->length);
}

/**
 * Serialize a packet into a uint8_t buffer, including our header information.
 * It is the caller's responsibility to free the serialization.
 * It is the caller's responsibility to free the packet.
 */
size_t serializePacket(const Packet *const packet, uint8_t **buffer) {
  // Total serialized length includes packet data and header
  size_t serializeLength = packet->length + PACKET_HEADER_LENGTH;
  assert(serializeLength <= MAX_PACKET_SIZE);

  uint8_t *data = (uint8_t *)malloc(serializeLength * sizeof(uint8_t));
  assert(data);

  // Setup the header
  uint32_t *dataAs32Bit = (uint32_t *)data;
  dataAs32Bit[0] = htonl(packet->seq);
  dataAs32Bit[1] = htonl(packet->ack);

  // Copy over the packet data
  memcpy(&data[PACKET_HEADER_LENGTH], packet->data, packet->length);

  *buffer = data;
  return serializeLength;
}

/**
 * Print the packet (for debugging)
 */
void printPacket(const Packet *const p) {
  printf("Packet:\n");
  printf("\tACK: %d\n\tSEQ: %d\n\tDATA (%zu bytes):", p->ack, p->seq,
         p->length);
  for (size_t i = 0; i < p->length; i++) {
    printf(" 0x%02x", p->data[i]);
  }
  printf("\n");
}

/**
 * Clean up a packet, deleting any data needed to be deleted.
 */
void freePacket(Packet *packet) {
  if (packet->data) {
    free(packet->data);
    packet->data = NULL;
  }
}

#endif  // LIB_PACKET_H
