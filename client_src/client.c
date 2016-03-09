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

#define SERVERPORT "4950"    // the port users will be connecting to

int main(int argc, char *argv[])
{
  int sockfd;
  struct addrinfo hints, *servinfo, *p;
  int rv;
  int numbytes;

  srand(time(NULL));

  if (argc != 4) {
    fprintf(stderr,"usage: talker <hostname> <port> <message>\n");
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
      perror("talker: socket");
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "talker: failed to create socket\n");
    return 2;
  }

  Config config;
  config.pC = 0.2;  // 80% chance of corruption
  config.pL = 0.2;  // 80% chance of packet loss
  config.windowSize = 5000;

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
  // TODO set filename
  fp = fopen("DOWNLOADEDFILE", "w");
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
