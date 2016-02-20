#include "rdtp.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "packet.h"

Buffer receiveBytes(int sockfd, struct sockaddr *fromAddress,
                    socklen_t *fromAddressLen) {
  ssize_t numBytesRec;
  uint8_t buffer[MAX_PACKET_SIZE];

  numBytesRec =
      recvfrom(sockfd, buffer, MAX_PACKET_SIZE, 0, fromAddress, fromAddressLen);
  assert(-1 != numBytesRec);  // TODO replace with proper error handling

  // TODO for now, assume one packet is sent. Work on multiple packets later!
  Packet rec;
  parsePacket(buffer, numBytesRec, &rec);
  printPacket(&rec);

  // TODO Send ACK, DO THIS RIGHT AWAY

  Buffer recBytes;
  recBytes.data = (uint8_t *)malloc(numBytesRec * sizeof(uint8_t));
  assert(recBytes.data);
  recBytes.length = numBytesRec;
  memcpy(recBytes.data, buffer, numBytesRec);

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

  // Serialize packet to send
  uint8_t *temp;
  size_t length = serializePacket(&packet, &temp);

  // Send the packet
  ssize_t numBytesSent =
      sendto(sockfd, temp, length, 0, destAddr, destLen);
  assert(-1 != numBytesSent); // TODO replace with proper error handling
  printf("%zd\n", numBytesSent);
  free(temp);

  printPacket(&packet);

  // TODO Handle ACK
}
