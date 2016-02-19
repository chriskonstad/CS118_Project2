#ifndef LIB_PACKET_H
#define LIB_PACKET_H

#include <assert.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * The Packet struct represents a packet's header and payload data.
 * The helper functions can be used to send and receive packets as byte arrays.
 */

const int MAX_PACKET_SIZE = 1024;    // number of bytes
const int PACKET_HEADER_LENGTH = 5;  // number of bytes

const int FLAG_ACK = 1 << 7;
const int FLAG_FIN = 1 << 6;

typedef struct Packet {
  bool isAck;     // ack flag
  bool isFin;     // fin flag
  uint32_t seq;   // seq number
  uint8_t *data;  // byte array received from socket, excluding header
  size_t length;  // length of data section
} Packet;

Packet makeAck(uint32_t seq) {
  Packet p;
  p.isAck = true;
  p.isFin = false;
  p.seq = seq;
  p.data = NULL;
  p.length = 0;
  return p;
}

Packet makeFin() {
  Packet p;
  p.isAck = false;
  p.isFin = true;
  p.seq = 0;
  p.data = NULL;
  p.length = 0;
  return p;
}

Packet makeFinAck() {
  Packet p;
  p.isAck = true;
  p.isFin = true;
  p.seq = 0;
  p.data = NULL;
  p.length = 0;
  return p;
}

/**
 * Read a byte array (a serialized packet) into a packet.
 * The packet must later be freed with freePacket, and the passed in
 * data array is freed.
 * The passed in data array is freed.
 */
void parsePacket(const uint8_t *const data, size_t length, Packet *packet) {
  // Parse flags
  const uint8_t flags = data[0];
  packet->isAck = flags & FLAG_ACK;
  packet->isFin = flags & FLAG_FIN;

  // Parse sequence number
  const uint32_t *dataAs32Bit = (uint32_t *)(&data[1]);
  packet->seq = ntohl(dataAs32Bit[0]);

  // Parse data
  packet->length = length - PACKET_HEADER_LENGTH;

  // Have the packet have its own copy of the data
  packet->data = (uint8_t *)malloc(packet->length * sizeof(uint8_t));
  assert(packet->data);
  memcpy(packet->data, (uint8_t *)&dataAs32Bit[1], packet->length);
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
  // Create flags
  uint8_t flags = 0;
  if (packet->isAck) {
    flags |= FLAG_ACK;
  }
  if (packet->isFin) {
    flags |= FLAG_FIN;
  }
  data[0] = flags;

  // Setup the sequence number
  uint32_t *dataAs32Bit = (uint32_t *)(&data[1]);
  dataAs32Bit[0] = htonl(packet->seq);

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
  printf("\tFLAG_ACK: %d\n\tFLAG_FIN: %d\n\tSEQ: %d\n\tDATA (%zu bytes):",
         p->isAck, p->isFin, p->seq, p->length);
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
