#include "buffer.h"

#include <stdlib.h>

void freeBuffer(Buffer *buf) {
  if (buf->data) {
    free(buf->data);
  }
}
