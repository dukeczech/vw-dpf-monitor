#ifndef ICON_DATA_H
#define ICON_DATA_H

#include <stdint.h>

struct tImage {
  const uint8_t* data;
  uint16_t width;
  uint16_t height;
  uint8_t dataSize;
};

#endif