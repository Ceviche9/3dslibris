#include <stdio.h>
#include <string>
#include <vector>

#include "library/cover_override_utils.h"

static bool Contains(const std::vector<std::string> &paths,
                     const char *expected) {
  for (size_t i = 0; i < paths.size(); ++i) {
    if (paths[i] == expected)
      return true;
  }
  return false;
}

int main() {
  std::vector<std::string> paths =
      cover_override_utils::BuildBookCandidates("sdmc:/books/Novel.epub");
  if (!Contains(paths, "sdmc:/books/Novel.jpg") ||
      !Contains(paths, "sdmc:/books/Novel.jpeg") ||
      !Contains(paths, "sdmc:/books/Novel.png")) {
    fprintf(stderr, "missing lowercase cover candidates\n");
    return 1;
  }
  if (!Contains(paths, "sdmc:/books/Novel.JPG") ||
      !Contains(paths, "sdmc:/books/Novel.JPEG") ||
      !Contains(paths, "sdmc:/books/Novel.PNG")) {
    fprintf(stderr, "missing uppercase cover candidates\n");
    return 1;
  }
  return 0;
}
