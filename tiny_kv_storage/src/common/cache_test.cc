// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (tobijah@163.com)
//

#include "cache.h"
#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <vector>

namespace tiny_kv {

TEST(LRUCacheTest, BasicOperations) {
  LRUCache<std::string, int> cache(3);

  cache.Put("key1", 1);
  cache.Put("key2", 2);

  EXPECT_EQ(cache.Get("key1"), 1);
  EXPECT_EQ(cache.Get("key2"), 2);
  EXPECT_FALSE(cache.Get("key3").has_value());

  cache.Put("key1", 100);
  EXPECT_EQ(cache.Get("key1"), 100);

  EXPECT_EQ(cache.Size(), 2);

  cache.Remove("key1");
  EXPECT_FALSE(cache.Get("key1").has_value());
  EXPECT_EQ(cache.Size(), 1);

  cache.Clear();
  EXPECT_EQ(cache.Size(), 0);
  EXPECT_FALSE(cache.Get("key2").has_value());
}

TEST(LRUCacheTest, LRUEviction) {
  LRUCache<std::string, int> cache(3);

  cache.Put("key1", 1);
  cache.Put("key2", 2);
  cache.Put("key3", 3);

  EXPECT_EQ(cache.Get("key1"), 1);

  cache.Put("key4", 4);

  // [ key4->key1->key3 ]
  EXPECT_EQ(cache.Size(), 3);
  EXPECT_TRUE(cache.Get("key1").has_value());
  EXPECT_FALSE(cache.Get("key2").has_value());
  EXPECT_TRUE(cache.Get("key3").has_value());
  EXPECT_TRUE(cache.Get("key4").has_value());

  // [ key4->key3->key1 ]
  cache.Put("key5", 5);

  // [ key5->key4->key3 ]
  EXPECT_EQ(cache.Size(), 3);
  EXPECT_FALSE(cache.Get("key1").has_value());
  EXPECT_TRUE(cache.Get("key3").has_value());
  EXPECT_TRUE(cache.Get("key4").has_value());
  EXPECT_TRUE(cache.Get("key5").has_value());
}

TEST(LRUCacheTest, ZeroCapacity) {
  LRUCache<std::string, int> cache(0);

  cache.Put("key1", 1);
  cache.Put("key2", 2);

  EXPECT_FALSE(cache.Get("key1").has_value());
  EXPECT_FALSE(cache.Get("key2").has_value());
  EXPECT_EQ(cache.Size(), 0);

  cache.Remove("key1"); // Should not crash
  cache.Clear();        // Should not crash
  EXPECT_EQ(cache.Capacity(), 0);
}

TEST(LRUCacheTest, EdgeCases) {
  LRUCache<std::string, int> cache(1);

  // Single capacity cache
  cache.Put("key1", 1);
  EXPECT_EQ(cache.Get("key1"), 1);

  // Replace the only element
  cache.Put("key2", 2);
  EXPECT_FALSE(cache.Get("key1").has_value());
  EXPECT_EQ(cache.Get("key2"), 2);

  // Remove non-existent key
  cache.Remove("key3");
  EXPECT_EQ(cache.Size(), 1);

  // Insert the same key
  cache.Put("key2", 200);
  EXPECT_EQ(cache.Size(), 1);
  EXPECT_EQ(cache.Get("key2"), 200);
}

TEST(LRUCacheTest, ThreadSafety) {
  LRUCache<std::string, int> cache(10);

  cache.Put("counter", 0);

  const int thread_count = 10;
  const int increments_per_thread = 1000;
  std::vector<std::thread> threads;

  for (int t = 0; t < thread_count; ++t) {
    threads.emplace_back([&cache]() {
      for (int i = 0; i < increments_per_thread; ++i) {
        auto value_opt = cache.Get("counter");
        int value = value_opt.value_or(0);
        cache.Put("counter", value + 1);
      }
    });
  }

  for (auto &thread : threads) {
    thread.join();
  }

  auto final_value = cache.Get("counter");
  EXPECT_TRUE(final_value.has_value());
  EXPECT_EQ(final_value.value(), thread_count * increments_per_thread);
}

TEST(LRUCacheTest, DifferentTypes) {
  LRUCache<std::string, std::string> str_cache(2);
  str_cache.Put("name", "value");
  EXPECT_EQ(str_cache.Get("name"), "value");

  LRUCache<int, std::string> int_str_cache(2);
  int_str_cache.Put(1, "one");
  EXPECT_EQ(int_str_cache.Get(1), "one");

  struct CustomType {
    int value;
  };

  LRUCache<int, CustomType> custom_cache(2);
  custom_cache.Put(1, CustomType{42});
  auto result = custom_cache.Get(1);
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result->value, 42);
}

} // namespace tiny_kv
