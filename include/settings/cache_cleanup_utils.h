#pragma once

namespace settings {

struct CacheCleanupResult {
  bool opened;
  int removed;
  int failed;

  CacheCleanupResult() : opened(false), removed(0), failed(0) {}
};

bool RemoveCacheEntry(const char *directory, const char *name);
CacheCleanupResult DeleteCacheDirectoryContents(const char *directory);

} // namespace settings
