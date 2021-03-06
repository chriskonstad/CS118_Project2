#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "../lib/rdtp.h"

int main(int argc, char *argv[])
{
  int sockfd;
  struct addrinfo hints, *servinfo, *p;
  int rv;
  struct sockaddr_storage their_addr;
  socklen_t addr_len;

  srand(time(NULL));

  if (argc != 2 && argc != 5 && argc != 7) {
    fprintf(stderr,"usage: server <port> optional: <corruption> <packet loss> <CWnd> (<timeout_sec> <timeout_usec>)\n");
    exit(1);
  }

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE; // use my IP

  if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  // loop through all the results and bind to the first we can
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
            p->ai_protocol)) == -1) {
      perror("server: socket");
      continue;
    }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("server: bind");
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "server: failed to bind socket\n");
    return 2;
  }

  freeaddrinfo(servinfo);

  printf("server: waiting to recvfrom...\n");

  addr_len = sizeof their_addr;

  Config config;
  if (argc == 5) {
    config.pC = atof(argv[2]);
    config.pL = atof(argv[3]);

    int windowSize = atoi(argv[4]);
    config.windowSize = windowSize;

    config.timeout_sec = 0;
    config.timeout_usec = 5000;
  } else if (argc == 7) {
    config.pC = atof(argv[2]);
    config.pL = atof(argv[3]);

    int windowSize = atoi(argv[4]);
    config.windowSize = windowSize;

    config.timeout_sec = atoi(argv[5]);
    config.timeout_usec = atoi(argv[6]);
  } else {
    // Default pC/pL/CWnd values
    config.pC = 0.8;  // 80% chance of corruption
    config.pL = 0.8;  // 80% chance of packet loss
    config.windowSize = 5000;

    config.timeout_sec = 0;
    config.timeout_usec = 5000;
  }

  //printf("WindowSize: %d\n", config.windowSize);
  Buffer rec;
  rec.data = NULL;
  rec.length = 0;

  // Will keep timing out while waiting for client to connect, so this will
  // keep waiting for a client to connect before continuing
  while (0 == rec.length) {
    rec = receiveBytes(sockfd, (struct sockaddr *)&their_addr, &addr_len, config);
  }

  // Ensure string
  rec.data = (uint8_t *)realloc(rec.data, rec.length + 1 * sizeof(uint8_t));
  rec.length++;
  rec.data[rec.length-1] = '\0';

  printf("Received %zu bytes\n", rec.length);
  printf("Client asked for file: %s\n", rec.data);

  // Open the requested file
  FILE *fp = fopen((const char *)rec.data, "r");

  // Default buffer to zero length
  // Send zero bytes in case of error
  Buffer fileBuffer;
  fileBuffer.length = 0;
  fileBuffer.data = 0;

  if (fp == NULL) {
    printf("Error: File %s cannot be found\n", rec.data);

    sendBytes(fileBuffer, sockfd, (struct sockaddr*)&their_addr, addr_len, config);

    exit(1);
  }

  char * fileBuf;
  long fileSize;
  size_t fileLength;

  // Load opened file into fileBuf
  if (!fseek(fp, 0L, SEEK_END)) { // Set stream position to end of file (SEEK_END)

    fileSize = ftell(fp);

    // Ensure valid size
    if (fileSize == -1) {
      printf("Error: File %s cannot be found\n", rec.data);

      sendBytes(fileBuffer, sockfd, (struct sockaddr*)&their_addr, addr_len, config);

      exit(1);
    }

    fileBuf = (char *)malloc(sizeof(char) * (fileSize+1));

    // Reset position to beginning
    if (fseek(fp, 0L, SEEK_SET)) {
      printf("Error: File seeking cannot be completed for %s\n", rec.data);

      sendBytes(fileBuffer, sockfd, (struct sockaddr*)&their_addr, addr_len, config);

      exit(1);
    }

    fileLength = fread(fileBuf, sizeof(char), fileSize, fp);

    printf("fileLength: %zu\n", fileLength);

    // Ensure non-zero length output
    if (!fileLength) {
      printf("Error: Cannot read file %s\n", rec.data);

      sendBytes(fileBuffer, sockfd, (struct sockaddr*)&their_addr, addr_len, config);

      exit(1);
    }

    fileBuf[fileLength] = '\0';

    Buffer fileBuffer;
    fileBuffer.data = (uint8_t *)fileBuf;
    fileBuffer.length = fileLength;
    sendBytes(fileBuffer, sockfd, (struct sockaddr *)&their_addr, addr_len, config);
  }

  fclose(fp);

  close(sockfd);

  return 0;
}
