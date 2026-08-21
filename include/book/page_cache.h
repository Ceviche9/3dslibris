/*
    3dslibris - page_cache.h
    LRU cache for computed layout pages
*/

#pragma once

#include "book/layout_engine.h"
#include "book/content_node.h"
#include <map>
#include <list>

class IStatusReporter;

namespace page_cache {

// Cache key based on page position and layout settings
struct CacheKey {
  size_t node_id;         // Use pointer as ID (assumes nodes don't move)
  size_t char_offset;
  int font_size;
  int margin_left;
  int margin_right;
  int screen_width;
  int screen_height;

  bool operator<(const CacheKey& other) const {
    if (node_id != other.node_id) return node_id < other.node_id;
    if (char_offset != other.char_offset) return char_offset < other.char_offset;
    if (font_size != other.font_size) return font_size < other.font_size;
    if (margin_left != other.margin_left) return margin_left < other.margin_left;
    if (margin_right != other.margin_right) return margin_right < other.margin_right;
    if (screen_width != other.screen_width) return screen_width < other.screen_width;
    return screen_height < other.screen_height;
  }
};

// LRU page cache
class PageCache {
public:
  PageCache();

  void SetMaxSize(size_t max_size);

  // Get page from cache or compute it. `reporter` is optional and only used
  // for DSLIBRIS_DEBUG logging.
  layout_engine::LayoutPage& GetPage(
    const layout_engine::PageStart& start,
    const layout_engine::LayoutMetrics& metrics,
    layout_engine::LayoutEngine* engine,
    IStatusReporter* reporter = nullptr
  );

  // Invalidate all cached pages
  void InvalidateAll();

  // Invalidate if metrics changed significantly
  void InvalidateIfChanged(const layout_engine::LayoutMetrics& new_metrics);

  size_t GetSize() const { return cache_.size(); }
  size_t GetMaxSize() const { return max_size_; }

private:
  CacheKey MakeKey(
    const layout_engine::PageStart& start,
    const layout_engine::LayoutMetrics& metrics
  ) const;

  void EvictOldest();

  size_t max_size_;
  layout_engine::LayoutMetrics last_metrics_;

  // Cache storage: key -> page
  std::map<CacheKey, layout_engine::LayoutPage> cache_;

  // LRU tracking: most recently used at front
  std::list<CacheKey> lru_list_;
  std::map<CacheKey, std::list<CacheKey>::iterator> lru_map_;
};

} // namespace page_cache
