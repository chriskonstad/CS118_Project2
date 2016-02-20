#include "rdtp.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "packet.h"

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

Buffer receiveBytes(int sockfd, struct sockaddr *fromAddress,
                    socklen_t *fromAddressLen) {
  // TODO for now, assume one packet is sent. Work on multiple packets later!
  // TODO also check packet flags to ensure it's a TRN packet
  Packet rec = receivePacket(sockfd, fromAddress, fromAddressLen);

  // Send ACK
  Packet ack = makeAck(rec.seq);
  sendPacket(&ack, sockfd, fromAddress, *fromAddressLen);

  // Turn received packets' data into a single buffer, and return that
  Buffer recBytes;
  recBytes.data = (uint8_t *)malloc(rec.length * sizeof(uint8_t));
  assert(recBytes.data);
  recBytes.length = rec.length;
  memcpy(recBytes.data, rec.data, rec.length);

  return recBytes;
}

void sendBytes(Buffer buf, int sockfd, const struct sockaddr *destAddr,
               socklen_t destLen) {
  // TODO for now, assume buf fits into one packet. Packetize it later!
  assert(buf.length <= MAX_PACKET_SIZE);

  // TODO Add proper error handling, resending, etc.
  Packet packet = makeTrn(118);  // change seq number
  packet.data = buf.data;
  packet.length = buf.length;

  sendPacket(&packet, sockfd, destAddr, destLen);

  // TODO Handle ACK
  Packet rec = receivePacket(sockfd, NULL, NULL);
  assert(rec.isAck);
}
