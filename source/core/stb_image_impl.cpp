// stb_image implementation file
// This compiles the stb_image library functions.
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_NO_FAILURE_STRINGS
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_GIF
#include "stb_image.h"

#include "shared/stb_image_gif_utils.h"

namespace stb_image_gif_utils {

unsigned char *LoadFromMemory(const unsigned char *buffer, int len, int *width,
                              int *height, int *channels,
                              int requested_channels) {
  if (!buffer || len <= 0 || !width || !height || !channels ||
      requested_channels < 0 || requested_channels > 4) {
    return NULL;
  }

  const bool is_gif = len >= 6 && buffer[0] == 'G' && buffer[1] == 'I' &&
                      buffer[2] == 'F' && buffer[3] == '8' &&
                      (buffer[4] == '7' || buffer[4] == '9') &&
                      buffer[5] == 'a';
  if (!is_gif) {
    return stbi_load_from_memory(buffer, len, width, height, channels,
                                 requested_channels);
  }

  stbi__context context;
  stbi__start_mem(&context, buffer, len);
  if (!stbi__gif_test(&context))
    return NULL;

  // stb_image's public GIF path keeps this 0x8858-byte decoder state on the
  // stack, which exceeds the 3DS thread stack used while rendering pages.
  stbi__gif *gif =
      static_cast<stbi__gif *>(STBI_MALLOC(sizeof(stbi__gif)));
  if (!gif)
    return NULL;
  memset(gif, 0, sizeof(*gif));

  stbi_uc *pixels = stbi__gif_load_next(&context, gif, channels,
                                        requested_channels, NULL);
  if (pixels == reinterpret_cast<stbi_uc *>(&context))
    pixels = NULL;

  if (pixels) {
    *width = gif->w;
    *height = gif->h;
    if (requested_channels && requested_channels != 4) {
      pixels = stbi__convert_format(pixels, 4, requested_channels, gif->w,
                                    gif->h);
    }
  } else if (gif->out) {
    STBI_FREE(gif->out);
  }

  STBI_FREE(gif->history);
  STBI_FREE(gif->background);
  STBI_FREE(gif);
  return pixels;
}

} // namespace stb_image_gif_utils
