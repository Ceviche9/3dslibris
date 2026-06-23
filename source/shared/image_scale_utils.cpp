#include "shared/image_scale_utils.h"

#include <stddef.h>

namespace image_scale_utils {

static uint16_t PackRgb565(unsigned r, unsigned g, unsigned b) {
  return (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
}

bool ScaleRgbToRgb565Bilinear(const unsigned char *src, int src_w, int src_h,
                              int dst_w, int dst_h, uint16_t *out) {
  if (!src || !out || src_w <= 0 || src_h <= 0 || dst_w <= 0 || dst_h <= 0)
    return false;

  for (int y = 0; y < dst_h; ++y) {
    const unsigned fy = dst_h > 1
        ? (unsigned)(((uint64_t)y * (uint64_t)(src_h - 1) << 16) /
                     (uint64_t)(dst_h - 1))
        : (unsigned)((src_h - 1) << 15);
    const int y0 = (int)(fy >> 16);
    const int y1 = y0 + 1 < src_h ? y0 + 1 : y0;
    const unsigned wy = fy & 0xFFFFu;
    for (int x = 0; x < dst_w; ++x) {
      const unsigned fx = dst_w > 1
          ? (unsigned)(((uint64_t)x * (uint64_t)(src_w - 1) << 16) /
                       (uint64_t)(dst_w - 1))
          : (unsigned)((src_w - 1) << 15);
      const int x0 = (int)(fx >> 16);
      const int x1 = x0 + 1 < src_w ? x0 + 1 : x0;
      const unsigned wx = fx & 0xFFFFu;
      const unsigned char *p00 = src + ((size_t)y0 * src_w + x0) * 3u;
      const unsigned char *p10 = src + ((size_t)y0 * src_w + x1) * 3u;
      const unsigned char *p01 = src + ((size_t)y1 * src_w + x0) * 3u;
      const unsigned char *p11 = src + ((size_t)y1 * src_w + x1) * 3u;
      unsigned rgb[3];
      for (int c = 0; c < 3; ++c) {
        const uint64_t top = (uint64_t)p00[c] * (65536u - wx) +
                             (uint64_t)p10[c] * wx;
        const uint64_t bottom = (uint64_t)p01[c] * (65536u - wx) +
                                (uint64_t)p11[c] * wx;
        rgb[c] = (unsigned)((top * (65536u - wy) + bottom * wy +
                             (1ULL << 31)) >> 32);
      }
      out[(size_t)y * dst_w + x] = PackRgb565(rgb[0], rgb[1], rgb[2]);
    }
  }
  return true;
}

} // namespace image_scale_utils
