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

extern const int MAX_PACKET_SIZE;    // number of bytes
extern const int PACKET_HEADER_LENGTH;  // number of bytes
extern const int MAX_SEQ_NUM;

extern const int FLAG_ACK;
extern const int FLAG_FIN;

typedef struct Packet {
  bool isAck;     // ack flag
  bool isFin;     // fin flag
  uint32_t seq;   // seq number
  uint8_t *data;  // byte array received from socket, excluding header
  size_t length;  // length of data section
} Packet;

// Caller must set data and length fields
Packet makeTrn(uint32_t seq);

Packet makeAck(uint32_t seq);

Packet makeFin();

Packet makeFinAck();

/**
 * Read a byte array (a serialized packet) into a packet.
 * The packet must later be freed with freePacket, and the passed in
 * data array is freed.
 * The passed in data array is freed.
 */
void parsePacket(const uint8_t *const data, size_t length, Packet *packet);

/**
 * Serialize a packet into a uint8_t buffer, including our header information.
 * It is the caller's responsibility to free the serialization.
 * It is the caller's responsibility to free the packet.
 */
size_t serializePacket(const Packet *const packet, uint8_t **buffer);

/**
 * Print the packet (for debugging)
 */
void printPacket(const Packet *const p);

/**
 * Clean up a packet, deleting any data needed to be deleted.
 */
void freePacket(Packet *packet);

#endif  // LIB_PACKET_H
