#include <stdio.h>

#include "shared/image_scale_utils.h"

int main() {
  const unsigned char rgb[] = {
      255, 0, 0, 0, 255, 0,
      0, 0, 255, 255, 255, 255,
  };
  uint16_t out = 0;
  if (!image_scale_utils::ScaleRgbToRgb565Bilinear(rgb, 2, 2, 1, 1, &out)) {
    fprintf(stderr, "bilinear scale failed\n");
    return 1;
  }
  if (out == 0xF800) {
    fprintf(stderr, "expected blended pixel, got 0x%04x\n",
            out);
    return 1;
  }
  return 0;
}
