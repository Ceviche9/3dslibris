#pragma once

#include <stdint.h>

namespace image_scale_utils {

bool ScaleRgbToRgb565Bilinear(const unsigned char *src, int src_w, int src_h,
                              int dst_w, int dst_h, uint16_t *out);

} // namespace image_scale_utils
