#ifndef LIB_BUFFER
#define LIB_BUFFER

#include <stdint.h>
#include <string.h>

/**
 * Generic buffer struct to store a byte array with a size.
 */
typedef struct Buffer {
  uint8_t *data;
  size_t length;
} Buffer;

void freeBuffer(Buffer *buf);

#endif  // LIB_BUFFER
