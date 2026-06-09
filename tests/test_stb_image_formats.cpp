#include "stb_image.h"

#include "test_assert.h"

namespace {

void TestGifInfoAndLoadAreEnabled() {
  static const unsigned char kGif1x1[] = {
      'G',  'I',  'F',  '8',  '9',  'a',  0x01, 0x00, 0x01, 0x00,
      0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x21,
      0xf9, 0x04, 0x01, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x00,
      0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x02, 0x02, 0x44,
      0x01, 0x00, 0x3b};

  int w = 0;
  int h = 0;
  int comp = 0;
  test::ExpectTrue("gif metadata supported",
                   stbi_info_from_memory(kGif1x1, sizeof(kGif1x1), &w, &h,
                                         &comp) != 0);
  test::ExpectEq("gif width", w, 1);
  test::ExpectEq("gif height", h, 1);

  unsigned char *pixels =
      stbi_load_from_memory(kGif1x1, sizeof(kGif1x1), &w, &h, &comp, 4);
  test::ExpectTrue("gif decode supported", pixels != NULL);
  stbi_image_free(pixels);
}

} // namespace

int main() {
  TestGifInfoAndLoadAreEnabled();
  return 0;
}
