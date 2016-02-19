#include <stdio.h>
#include "packet.h"

/**
 * This is a test program to test how the packet library works, and to serve
 * as example code.
 */

int main()
{
  // Build test packet to pretend to send
  char *testData = "hello world!";
  Packet send;
  send.ack = 118;
  send.seq = 229;
  send.data = (uint8_t*)testData;
  send.length = 13;

  printf("SENDING:\n");
  printPacket(&send);

  // Serialize the packet
  uint8_t *udp; // must free later
  size_t udpLength = serializePacket(&send, &udp);

  printf("\nSerialized packet (%zu bytes):\n", udpLength);
  for(size_t i=0; i<udpLength; i++) {
    printf("0x%02x ", udp[i]);
  }
  printf("\n\n");

  // Pretend to receive the packet
  printf("RECEIVING:\n");
  Packet rec; // must free later
  parsePacket(udp, udpLength, &rec);
  printPacket(&rec);

  // Clean up
  // Don't need to free 'send' packet because the data wasn't malloc'd
  freePacket(&rec);
  free(udp);
}
