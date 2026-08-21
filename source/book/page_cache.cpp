/*
    3dslibris - page_cache.cpp
    Page cache implementation
*/

#include "book/page_cache.h"
#include "shared/debug_log.h"

namespace page_cache {

PageCache::PageCache()
  : max_size_(5)
{
}

void PageCache::SetMaxSize(size_t max_size) {
  max_size_ = max_size;

  // Evict excess pages
  while (cache_.size() > max_size_) {
    EvictOldest();
  }
}

layout_engine::LayoutPage& PageCache::GetPage(
  const layout_engine::PageStart& start,
  const layout_engine::LayoutMetrics& metrics,
  layout_engine::LayoutEngine* engine,
  IStatusReporter* reporter
) {
  CacheKey key = MakeKey(start, metrics);

  // Check if in cache
  auto it = cache_.find(key);
  if (it != cache_.end()) {
    // Move to front of LRU
    auto lru_it = lru_map_[key];
    lru_list_.erase(lru_it);
    lru_list_.push_front(key);
    lru_map_[key] = lru_list_.begin();

    return it->second;
  }

  if (!engine) {
    DBG_LOGF_CAT(reporter, DBG_LEVEL_ERROR, DBG_CAT_LAYOUT,
                 "PAGE-CACHE: miss node=%p offset=%u but engine=null",
                 (void*)start.node, (unsigned)start.char_offset);
    static layout_engine::LayoutPage empty_page;
    return empty_page;
  }

  DBG_LOGF_CAT(reporter, DBG_LEVEL_DEBUG, DBG_CAT_LAYOUT,
               "PAGE-CACHE: miss node=%p offset=%u size=%u/%u",
               (void*)start.node, (unsigned)start.char_offset,
               (unsigned)cache_.size(), (unsigned)max_size_);

  // Not in cache - compute page
  layout_engine::LayoutPage page = engine->ComputePage(start, metrics, reporter);

  // Add to cache
  cache_[key] = page;
  lru_list_.push_front(key);
  lru_map_[key] = lru_list_.begin();

  // Evict if over limit
  if (cache_.size() > max_size_) {
    EvictOldest();
  }

  return cache_[key];
}

void PageCache::InvalidateAll() {
  cache_.clear();
  lru_list_.clear();
  lru_map_.clear();
}

void PageCache::InvalidateIfChanged(const layout_engine::LayoutMetrics& new_metrics) {
  // Check if significant settings changed
  bool changed = false;

  if (last_metrics_.screen_width != new_metrics.screen_width ||
      last_metrics_.screen_height != new_metrics.screen_height ||
      last_metrics_.base_margin_left != new_metrics.base_margin_left ||
      last_metrics_.base_margin_right != new_metrics.base_margin_right ||
      last_metrics_.base_margin_top != new_metrics.base_margin_top ||
      last_metrics_.base_margin_bottom != new_metrics.base_margin_bottom ||
      last_metrics_.line_spacing != new_metrics.line_spacing) {
    changed = true;
  }

  if (changed) {
    InvalidateAll();
    last_metrics_ = new_metrics;
  }
}

CacheKey PageCache::MakeKey(
  const layout_engine::PageStart& start,
  const layout_engine::LayoutMetrics& metrics
) const {
  CacheKey key;
  key.node_id = reinterpret_cast<size_t>(start.node);
  key.char_offset = start.char_offset;
  key.font_size = 16; // TODO: get from metrics
  key.margin_left = metrics.base_margin_left;
  key.margin_right = metrics.base_margin_right;
  key.screen_width = metrics.screen_width;
  key.screen_height = metrics.screen_height;
  return key;
}

void PageCache::EvictOldest() {
  if (lru_list_.empty()) return;

  // Remove oldest (back of list)
  CacheKey oldest = lru_list_.back();
  lru_list_.pop_back();
  lru_map_.erase(oldest);
  cache_.erase(oldest);
}

} // namespace page_cache
