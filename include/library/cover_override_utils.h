#pragma once

#include <string>
#include <vector>

namespace cover_override_utils {

std::vector<std::string> BuildBookCandidates(const std::string &book_path);
std::vector<std::string> BuildStemCandidates(const std::string &stem_path);

} // namespace cover_override_utils
