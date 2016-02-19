#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include "packet.h"

/**
 * This is a test program to test how the packet library works, and to serve
 * as example code.
 */

bool comparePackets(Packet *a, Packet *b) {
  if(a->isAck != b->isAck) return false;
  if(a->isFin != b->isFin) return false;
  if(a->seq != b->seq) return false;
  if(a->length != b->length) return false;

  for(size_t i=0; i<a->length; i++) {
    if(a->data[i] != b->data[i]) {
      return false;
    }
  }

  return true;
}

void pretendSend(Packet *p) {
  uint8_t *buffer;
  size_t length = serializePacket(p, &buffer);

  Packet rec;
  parsePacket(buffer, length, &rec);
  printf("\nSent:\n");
  printPacket(p);
  printf("Received:\n");
  printPacket(p);
  printf("\n");

  // Fail the test if the packets are different after being serialized
  assert(comparePackets(p, &rec));

  freePacket(&rec);
  free(buffer);
}

int main()
{
  // TEST PACKET TYPES
  // TRN
  char *testData = "hello world!";
  Packet trn;
  trn.isAck = false;
  trn.isFin = false;
  trn.seq = 118;
  trn.data = (uint8_t*)testData;
  trn.length = 13;
  // Don't need to free 'trn' packet because the data wasn't malloc'd
  // However, usually will need to free it

  pretendSend(&trn);
  // ACK
  Packet ack = makeAck(118);
  pretendSend(&ack);
  freePacket(&ack);

  // FIN
  Packet fin = makeFin();
  pretendSend(&fin);
  freePacket(&fin);

  // FINACK
  Packet finAck = makeFinAck();
  pretendSend(&finAck);
  freePacket(&finAck);
}
