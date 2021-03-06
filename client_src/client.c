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

  srand(time(NULL));

  if (argc != 4 && argc != 7 && argc != 9) {
    fprintf(stderr,"usage: client <hostname> <port> <filename> optional: <corruption> <packet loss> <CWnd> (<timeout_sec> <timeout_usec>)\n");
    exit(1);
  }

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;

  char * port = argv[2];

  if ((rv = getaddrinfo(argv[1], port, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  // loop through all the results and make a socket
  for(p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
            p->ai_protocol)) == -1) {
      perror("client: socket");
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "client: failed to create socket\n");
    return 2;
  }

  Config config;
  if (argc == 7) {
    config.pC = atof(argv[4]);
    config.pL = atof(argv[5]);

    int windowSize = atoi(argv[6]);
    //printf("WindowSize: %d\n", windowSize);
    config.windowSize = windowSize;

    config.timeout_sec = 0;
    config.timeout_usec = 5000;
  } else if (argc == 9) {
    config.pC = atof(argv[4]);
    config.pL = atof(argv[5]);

    int windowSize = atoi(argv[6]);
    //printf("WindowSize: %d\n", windowSize);
    config.windowSize = windowSize;

    config.timeout_sec = atoi(argv[7]);
    config.timeout_usec = atoi(argv[8]);

  } else {
    // Default pC/pL values
    config.pC = 0.0;  // 00% chance of corruption
    config.pL = 0.0;  // 00% chance of packet loss
    config.windowSize = 5000;

    config.timeout_sec = 0;
    config.timeout_usec = 5000;
  }

  Buffer buffer;
  buffer.data = (uint8_t*)argv[3];
  buffer.length = strlen(argv[3]);
  bool asked = sendBytes(buffer, sockfd, p->ai_addr, p->ai_addrlen, config);
  if(!asked) {
    printf("Unable to reach server.\n");
    exit(1);
  }

  printf("Asked for file %s\n", argv[3]);

  // Receive file back from server here
  Buffer downloadedFile = receiveBytes(sockfd, p->ai_addr, &p->ai_addrlen, config);
  if(downloadedFile.length == 0) {
    printf("Received no bytes. Exiting.\n");
    exit(1);
  }
  FILE *fp;

  char downloadedFileName[4096];
  strcpy(downloadedFileName, "DL_");
  strcat(downloadedFileName, argv[3]);

  //printf("Name of file: %s", downloadedFileName);

  fp = fopen(downloadedFileName, "w");
  if (fp == NULL) {
    printf("Error: File %s cannot be written!\n", argv[3]);
    exit(1);
  }

  size_t bytesWritten = fwrite(downloadedFile.data,
                               sizeof(uint8_t),
                               downloadedFile.length,
                               fp);
  fclose(fp);

  if(bytesWritten != downloadedFile.length) {
    printf("Error: Did not write out entire file!\n");
    exit(1);
  }

  freeaddrinfo(servinfo);

  close(sockfd);

  return 0;
}
