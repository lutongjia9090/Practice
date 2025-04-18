// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (tobijah@163.com)
//

#pragma once

#include <cstddef>
#include <list>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <utility>

namespace tiny_kv {

/************************************************************************/
/* LRUCache */
/************************************************************************/
template <typename K, typename V> class LRUCache {
public:
  explicit LRUCache(size_t capacity) : capacity_(capacity) {}
  ~LRUCache() { Clear(); }

  std::optional<V> Get(const K &key) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cache_map_.find(key);
    if (it == cache_map_.end()) {
      return std::nullopt;
    }

    cache_list_.splice(cache_list_.begin(), cache_list_, it->second.list_iter);
    return it->second.value;
  }

  void Put(const K &key, const V &value) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (capacity_ == 0) {
      return;
    }

    auto it = cache_map_.find(key);
    if (it != cache_map_.end()) {
      it->second.value = value;
      cache_list_.splice(cache_list_.begin(), cache_list_,
                         it->second.list_iter);
      return;
    }

    if (cache_list_.size() >= capacity_ && !cache_list_.empty()) {
      const K &old_key = cache_list_.back();
      cache_map_.erase(old_key);
      cache_list_.pop_back();
    }

    cache_list_.push_front(key);

    CacheItem item;
    item.value = value;
    item.list_iter = cache_list_.begin();
    cache_map_[key] = std::move(item);
  }

  void Remove(const K &key) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = cache_map_.find(key);
    if (it != cache_map_.end()) {
      cache_list_.erase(it->second.list_iter);
      cache_map_.erase(it);
    }
  }

  void Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    cache_list_.clear();
    cache_map_.clear();
  }

  size_t Size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return cache_list_.size();
  }

  size_t Capacity() const { return capacity_; }

private:
  struct CacheItem {
    V value;
    typename std::list<K>::iterator list_iter;
  };

  size_t capacity_;
  std::list<K> cache_list_;
  std::unordered_map<K, CacheItem> cache_map_;
  mutable std::mutex mutex_;
};
} // namespace tiny_kv
