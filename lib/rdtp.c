#include "rdtp.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>

#include "packet.h"

const int TIMEOUT_SEC = 0;
const int TIMEOUT_USEC = 100;
const int MAX_FIN_ATTEMPT = 5;

typedef enum { OK, CORRUPTED, TIMEDOUT } STATUS;

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
                     socklen_t *fromAddressLen, STATUS *status,
                     Config config) {
  *status = OK;
  uint8_t buffer[MAX_PACKET_SIZE];

  // Enable timeout
  struct timeval tv;
  tv.tv_sec = TIMEOUT_SEC;  // 3 second timeout
  tv.tv_usec = TIMEOUT_USEC;
  fd_set sockets;
  FD_ZERO(&sockets);
  FD_SET(sockfd, &sockets);

  Packet ret;
  if (select(sockfd + 1, &sockets, NULL, NULL, &tv)) {
    ssize_t bytesRec = recvfrom(sockfd, buffer, MAX_PACKET_SIZE, 0, fromAddress,
                                fromAddressLen);
    assert(-1 != bytesRec);  // TODO replace with proper error handling later
    // Parse out the packet
    parsePacket(buffer, bytesRec, &ret);

    // Print packet for debugging
    printPacket(&ret);
    // Check if corrupted
    int r = (rand() % 100) + 1; // [1,100]
    *status = r <= (config.pC * 100) ? OK : CORRUPTED;
    if(*status == CORRUPTED) {
      printf("\x1B[31m" "\t(CORRUPTED)\n" "\x1B[0m");
    }
  } else {
    // Timed out
    *status = TIMEDOUT;
    printf(
        "\x1B[31m"
        "\t(TIMED OUT)\n"
        "\x1B[0m");
  }

  return ret;
}

/**
 * Receive a byte array
 */
Buffer receiveBytes(int sockfd, struct sockaddr *fromAddress,
                    socklen_t *fromAddressLen, Config config) {
  // TODO also check packet flags to ensure it's a TRN packet
  Buffer recBytes;
  recBytes.data = NULL;
  recBytes.length = 0;

  while (1) {
    STATUS status;
    Packet rec =
        receivePacket(sockfd, fromAddress, fromAddressLen, &status, config);

    // Ignore corrupted packets
    if(status != OK) {
      continue;
    }

    // Handle different packet types
    if (!rec.isAck && !rec.isFin) {
      // Copy packet data into recBytes
      if(rec.seq == recBytes.length) {
        recBytes.data =
            (uint8_t *)realloc(recBytes.data, recBytes.length + rec.length);
        assert(recBytes.data);
        memcpy(&recBytes.data[recBytes.length], rec.data, rec.length);
        recBytes.length += rec.length;
      }

      // Send ACK
      Packet ack = makeAck(rec.seq);
      freePacket(&rec); // only need to free TRN packets

      sendPacket(&ack, sockfd, fromAddress, *fromAddressLen);
    } else if (!rec.isAck && rec.isFin) {
      // Send FINACK
      Packet finAck = makeFinAck();
      sendPacket(&finAck, sockfd, fromAddress, *fromAddressLen);
      break;
      // TODO Handle rest of the ending handshake
      // TODO There is a bug where the fin is not received, causing this to
      //      loop forever. Fix!  Do the two way fin-finack handshake like TCP
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
               socklen_t destLen, Config config) {
  // Packetize the data
  Packet *packets;
  int numPackets = packetize(buf, &packets);
  STATUS status;

  for (int i = 0; i < numPackets; i++) {
    Packet rec;
    do {
      // Send packet
      sendPacket(&packets[i], sockfd, destAddr, destLen);

      // Handle ACK
      rec = receivePacket(sockfd, NULL, NULL, &status, config);
    } while (status != OK);
    assert(rec.isAck);  // TODO replace with real error handling
  }

  // Send FIN
  Packet finAck;
  int attempts = 0;
  do {
    attempts++;
    Packet fin = makeFin();
    sendPacket(&fin, sockfd, destAddr, destLen);

    // Handle FINACK
    finAck = receivePacket(sockfd, NULL, NULL, &status, config);
  } while(status != OK && attempts < MAX_FIN_ATTEMPT);
}
