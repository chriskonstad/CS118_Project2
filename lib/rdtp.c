#include "rdtp.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "packet.h"

/**
 * Send a singular packet of any type
 */
void sendPacket(Packet *p, int sockfd, const struct sockaddr *destAddr,
                socklen_t destLen) {
  // Print for debugging
  printPacket(p);

  // Serialize the packet
  uint8_t *temp;
  size_t length = serializePacket(p, &temp);

  // Send the packet
  ssize_t bytesSent = sendto(sockfd, temp, length, 0, destAddr, destLen);

  // Clean up
  free(temp);

  // Handle errors
  assert(-1 != bytesSent);  // TODO replace with proper error handling later
}

/**
 * Receive a singular packet of any type
 */
Packet receivePacket(int sockfd, struct sockaddr *fromAddress,
                     socklen_t *fromAddressLen) {
  uint8_t buffer[MAX_PACKET_SIZE];

  ssize_t bytesRec =
      recvfrom(sockfd, buffer, MAX_PACKET_SIZE, 0, fromAddress, fromAddressLen);
  assert(-1 != bytesRec);  // TODO replace with proper error handling later

  // Parse out the packet
  Packet ret;
  parsePacket(buffer, bytesRec, &ret);

  // Print packet for debugging
  printPacket(&ret);

  return ret;
}

/**
 * Receive a byte array
 */
Buffer receiveBytes(int sockfd, struct sockaddr *fromAddress,
                    socklen_t *fromAddressLen) {
  // TODO also check packet flags to ensure it's a TRN packet
  Buffer recBytes;
  recBytes.data = NULL;
  recBytes.length = 0;

  while (1) {
    Packet rec = receivePacket(sockfd, fromAddress, fromAddressLen);

    // Handle different packet types
    if (!rec.isAck && !rec.isFin) {
      // Copy packet data into recBytes
      recBytes.data =
          (uint8_t *)realloc(recBytes.data, recBytes.length + rec.length);
      assert(recBytes.data);
      memcpy(&recBytes.data[recBytes.length], rec.data, rec.length);
      recBytes.length += rec.length;

      // Send ACK
      Packet ack = makeAck(rec.seq);
      sendPacket(&ack, sockfd, fromAddress, *fromAddressLen);
    } else if (!rec.isAck && rec.isFin) {
      // Send FINACK
      Packet finAck = makeFinAck();
      sendPacket(&finAck, sockfd, fromAddress, *fromAddressLen);
      break;
      // TODO Handle rest of the ending handshake
    }
    // TODO Handle rest of the packet types
  }

  return recBytes;
}

/**
 * Calculate how hany packets are required to send a buffer
 */
int numPacketsRequired(Buffer buf) {
  int base = buf.length / MAX_PACKET_DATA;
  int mod = buf.length % MAX_PACKET_DATA;
  if (mod) {
    base++;
  }
  return base;
}

/**
 * Create an array of TRN packets that point into buf
 * The caller should NOT free the packets, as they point into buf.
 * The caller should free the packet list, though.
 */
int packetize(Buffer buf, Packet **packets) {
  int numPackets = numPacketsRequired(buf);
  *packets = (Packet *)malloc(numPackets * sizeof(Packet));
  assert(0 != *packets);
  size_t dataOffset = 0;
  for (int i = 0; i < numPackets; i++) {
    (*packets)[i] = makeTrn(dataOffset);
    (*packets)[i].data = &buf.data[dataOffset];
    if (i < (numPackets - 1)) {
      (*packets)[i].length = MAX_PACKET_DATA;
    } else {
      (*packets)[i].length = buf.length - dataOffset;
    }
    dataOffset += (*packets)[i].length;
  }

  return numPackets;
}

/**
 * Send a byte array
 */
void sendBytes(Buffer buf, int sockfd, const struct sockaddr *destAddr,
               socklen_t destLen) {
  // Packetize the data
  Packet *packets;
  int numPackets = packetize(buf, &packets);

  for (int i = 0; i < numPackets; i++) {
    // TODO Add proper error handling, resending, etc.
    // Send packet
    sendPacket(&packets[i], sockfd, destAddr, destLen);

    // Handle ACK
    Packet rec = receivePacket(sockfd, NULL, NULL);
    assert(rec.isAck);  // TODO replace with real error handling
  }

  // Send FIN
  Packet fin = makeFin();
  sendPacket(&fin, sockfd, destAddr, destLen);

  // Handle FINACK
  Packet finAck = receivePacket(sockfd, NULL, NULL);
  assert(finAck.isAck);  // TODO replace with real error handling
  assert(finAck.isFin);  // TODO replace with real error handling
}
