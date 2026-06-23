#include "library/cover_override_utils.h"

namespace cover_override_utils {

std::vector<std::string> BuildStemCandidates(const std::string &stem_path) {
  std::vector<std::string> result;
  if (stem_path.empty() || stem_path[stem_path.size() - 1] == '/')
    return result;
  const char *extensions[] = {".jpg", ".jpeg", ".png", ".JPG", ".JPEG", ".PNG"};
  for (size_t i = 0; i < sizeof(extensions) / sizeof(extensions[0]); ++i)
    result.push_back(stem_path + extensions[i]);
  return result;
}

std::vector<std::string> BuildBookCandidates(const std::string &book_path) {
  const size_t slash = book_path.find_last_of('/');
  const size_t dot = book_path.find_last_of('.');
  const size_t stem_end = dot != std::string::npos &&
                                  (slash == std::string::npos || dot > slash)
                              ? dot
                              : book_path.size();
  return BuildStemCandidates(book_path.substr(0, stem_end));
}

} // namespace cover_override_utils
