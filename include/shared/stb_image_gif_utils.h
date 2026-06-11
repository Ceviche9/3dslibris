#pragma once

namespace stb_image_gif_utils {

unsigned char *LoadFromMemory(const unsigned char *buffer, int len, int *width,
                              int *height, int *channels,
                              int requested_channels);

} // namespace stb_image_gif_utils
