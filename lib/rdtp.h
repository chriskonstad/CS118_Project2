#ifndef LIB_RDTP
#define LIB_RDTP

#include <netinet/in.h>
#include <stdbool.h>

#include "buffer.h"

/**
 * Configuration struct for tweaking the parameters of RDTP
 */
typedef struct Config {
  double pC;
  double pL;
  int windowSize;
} Config;

/**
 * recvfrom
 * NEED:
 * sockfd
 * buffer
 * bufferlength
 * flags
 * sockaddr address (their address)
 * socklen_t address_len,
 */
Buffer receiveBytes(int sockfd, struct sockaddr *restrict fromAddress,
                    socklen_t *restrict fromAddressLen, Config config);

bool sendBytes(Buffer buf, int sockfd, const struct sockaddr *destAddr,
               socklen_t destLen, Config config);

#endif  // LIB_RDTP
