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

// Keep the timeout as small as possible to increase transfer rate
const int TIMEOUT_SEC = 0;
const int TIMEOUT_USEC = 5000;  // 5 millisecond
const int MAX_FIN_ATTEMPT = 50;

// Number of times the receiver will receive TIMEDOUT before assuming
// loss of connection.
const int MAX_WAIT_ATTEMPTS = 500;
const int MAX_SEND_ATTEMPTS = 500;

typedef enum { OK, CORRUPTED, TIMEDOUT, LOST } STATUS;

// Struct to represent a window
typedef struct Window {
  size_t min;
  size_t max;
} Window;

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
  if(-1 == bytesSent) {
    printf("sendPacket Error: %s\n", strerror(errno));
  }
}

/**
 * Receive a singular packet of any type
 */
Packet receivePacket(int sockfd, struct sockaddr *fromAddress,
                     socklen_t *fromAddressLen, STATUS *status,
                     Config config, bool isFirstPacket) {
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
    *status = r <= (config.pC * 100) ? CORRUPTED : OK;
    *status = r <= (config.pL * 100) ? LOST : *status;
    if(*status == CORRUPTED) {
      printf("\x1B[31m" "\t(CORRUPTED)\n" "\x1B[0m");
    } else if(*status == LOST) {
      printf("\x1B[31m" "\t(LOST)\n" "\x1B[0m");
    }
  } else {
    // Timed out
    *status = TIMEDOUT;

    // Don't 'time out' if simply waiting for the first packet
    if(!isFirstPacket) {
      printf(
          "\x1B[31m"
          "\t(TIMED OUT)\n"
          "\x1B[0m");
    }
  }

  return ret;
}

/**
 * Receive a byte array
 */
Buffer receiveBytes(int sockfd, struct sockaddr *fromAddress,
                    socklen_t *fromAddressLen, Config config) {
  Buffer recBytes;
  recBytes.data = NULL;
  recBytes.length = 0;

  int timeOuts = 0;
  bool isFirstPacket = true;

  int numRollovers = 0; // number of times seq rolls over MAX_SEQ_NUM

  while (1) {
    STATUS status;
    Packet rec =
        receivePacket(sockfd, fromAddress, fromAddressLen, &status, config, isFirstPacket);

    // Eat finacks from old connection
    if(rec.isFin && rec.isAck) {
      continue;
    }

    // Handle loss of connection
    if(TIMEDOUT  == status && !isFirstPacket) {
      timeOuts++;
      if(MAX_WAIT_ATTEMPTS < timeOuts) {
        printf(
            "\x1B[31m"
            "\t(Connection Lost)\n"
            "\x1B[0m");
        break;
      }
    }

    // If we RXed a packet, then reset the timeout counter
    if(CORRUPTED == status || OK == status) {
      isFirstPacket = false;
      timeOuts = 0;
    }

    // Ignore corrupted packets
    if(status != OK) {
      continue;
    }

    // Handle different packet types
    if (!rec.isAck && !rec.isFin) {
      // Copy packet data into recBytes
      printf("SEQ: %d, LENGTH: %d\n", rec.seq, recBytes.length);
      if(rec.seq < recBytes.length) {
        numRollovers++;
      }
      if(rec.seq + (numRollovers * MAX_SEQ_NUM) == recBytes.length) {
        recBytes.data =
            (uint8_t *)realloc(recBytes.data, recBytes.length + rec.length);
        assert(recBytes.data);
        memcpy(&recBytes.data[recBytes.length], rec.data, rec.length);
        recBytes.length += rec.length;
      }

      // Send ACK
      Packet ack = makeAck(rec.seq);
      freePacket(&rec); // only need to free TRN packets

      assert(status == OK);
      sendPacket(&ack, sockfd, fromAddress, *fromAddressLen);
    } else if (!rec.isAck && rec.isFin) {
      // Send FINACK
      Packet finAck = makeFinAck();
      sendPacket(&finAck, sockfd, fromAddress, *fromAddressLen);
      break;
    }
  }

  return recBytes;
}

/**
 * Calculate how many packets are required to send a buffer
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
    (*packets)[i] = makeTrn(dataOffset % MAX_SEQ_NUM);
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
bool sendBytes(Buffer buf, int sockfd, const struct sockaddr *destAddr,
               socklen_t destLen, Config config) {
  // Packetize the data
  Packet *packets;
  int numPackets = packetize(buf, &packets);
  STATUS status;
  int sendAttempts = 0;

  // Eat old connection's FIN packets
  Packet oldFin;
  do {
    oldFin = receivePacket(sockfd, NULL, NULL, &status, config, false);
    if(OK == status) {
      assert(oldFin.isFin);
    }
  } while(TIMEDOUT != status);

  // Send bytes
  for (int i = 0; i < numPackets; i++) {
    Packet rec;
    sendAttempts = 0;
    do {
      // If nobody is listening, let the sender timeout
      if(MAX_SEND_ATTEMPTS < sendAttempts) {
        return false;
      }
      sendAttempts++;

      // Send packet
      sendPacket(&packets[i], sockfd, destAddr, destLen);

      // Handle ACK
      rec = receivePacket(sockfd, NULL, NULL, &status, config, false);
    } while (status != OK);

    if(!rec.isAck) {
      i--;  // resend this packet
    }
  }

  // Send FIN
  Packet finAck;
  int attempts = 0;
  do {
    attempts++;
    Packet fin = makeFin();
    sendPacket(&fin, sockfd, destAddr, destLen);

    // Handle FINACK
    finAck = receivePacket(sockfd, NULL, NULL, &status, config, false);

    // If other side started transmitting
    if(OK == status && !finAck.isAck && !fin.isFin) {
      break;
    }
  } while(status != OK && attempts < MAX_FIN_ATTEMPT);

  return true;
}
