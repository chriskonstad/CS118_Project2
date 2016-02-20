#ifndef LIB_RDTP
#define LIB_RDTP

#include <netinet/in.h>

#include "buffer.h"

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
                    socklen_t *restrict fromAddressLen);

void sendBytes(Buffer buf, int sockfd, const struct sockaddr *destAddr,
               socklen_t destLen);

#endif  // LIB_RDTP
