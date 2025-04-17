// Copyright 2024 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (lutonjia@163.com)
//

#include "absl/container/flat_hash_map.h"
#include "absl/container/node_hash_map.h"
#include <ankerl/unordered_dense.h>
#include <array>
#include <chrono>
#include <parallel_hashmap/phmap.h>
#include <random>
#include <stdio.h>
#include <string>
#include <unordered_map>
#include <vector>

/************************************************************************/
/* Macros */
/************************************************************************/
#define SMART_BENCHMARK_DO_NOT_OPTIMIZE(x)                                     \
  __asm__ __volatile__("" ::"g"(x) : "memory")

#define PRINT_TABLE_HEADER(Header)                                             \
  for (size_t i = 0; i < Header.size(); ++i) {                                 \
    printf("%s", Header[i].c_str());                                           \
    if (i == Header.size() - 1) {                                              \
      printf("\n");                                                            \
    } else {                                                                   \
      printf("\t");                                                            \
    }                                                                          \
  }

#define PRINT_TABLE_DATA(Data)                                                 \
  for (size_t i = 0; i < Data.size(); ++i) {                                   \
    printf("%f", Data[i]);                                                     \
    if (i == Data.size() - 1) {                                                \
      printf("\n");                                                            \
    } else {                                                                   \
      printf("\t");                                                            \
    }                                                                          \
  }

/************************************************************************/
/* Benchmark */
/************************************************************************/
template <class Key, class Value, class HashMap> class Benchmark {
private:
  HashMap hash_map_;
  std::vector<Key> ids_;

public:
  void SetUp(size_t size) {
    hash_map_.reserve(size);
    ids_.reserve(size);

    std::default_random_engine engine;
    std::uniform_int_distribution<Key> dist;

    for (size_t i = 0; i < size; ++i) {
      while (true) {
        Key key = dist(engine);
        Value value;
        ids_.emplace_back(key);
        auto res = hash_map_.emplace(key, std::move(value));
        if (res.second) {
          break;
        }
      }
    }
  }

  double Find() const {
    auto begin_time = std::chrono::steady_clock::now();
    for (size_t i = 0; i < ids_.size(); ++i) {
      auto it = hash_map_.find(ids_[i]);
      // Avoid Dead-code elimination
      SMART_BENCHMARK_DO_NOT_OPTIMIZE(it);
    }

    auto end_time = std::chrono::steady_clock::now();
    auto duration = end_time - begin_time;
    double nanoseconds =
        (double)std::chrono::duration_cast<std::chrono::nanoseconds>(duration)
            .count();
    return nanoseconds / (double)ids_.size();
  }
};

/************************************************************************/
/* TestSuite */
/************************************************************************/
template <typename Key, class Value> void TestSuite() {
  constexpr size_t SIZE = 1000000;
  std::vector<double> duration;
  std::vector<std::string> header;

#define TEST_HASHMAP(HashMap)                                                  \
  do {                                                                         \
    Benchmark<Key, Value, HashMap<Key, Value>> b;                              \
    b.SetUp(SIZE);                                                             \
    double e = b.Find();                                                       \
    header.emplace_back(#HashMap);                                             \
    duration.emplace_back(e);                                                  \
  } while (0);

  TEST_HASHMAP(std::unordered_map);
  TEST_HASHMAP(absl::flat_hash_map);
  TEST_HASHMAP(absl::node_hash_map);
  TEST_HASHMAP(phmap::flat_hash_map);
  TEST_HASHMAP(phmap::parallel_flat_hash_map);
  TEST_HASHMAP(ankerl::unordered_dense::map);
  // Add hashmap here.

  PRINT_TABLE_HEADER(header);
  PRINT_TABLE_DATA(duration);
}

// `std::vector` with initial size
template <class T, int DIM> class Vector : public std::vector<T> {
public:
  using Base = std::vector<T>;
  Vector() : Base(DIM) {}
};

int main(int /*argc*/, char ** /*argv*/) {
  // DIM 8
  TestSuite<uint64_t, Vector<float, 8>>();
  TestSuite<uint64_t, std::array<float, 8>>();
  return 0;
}
