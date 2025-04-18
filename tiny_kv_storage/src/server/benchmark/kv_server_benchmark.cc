// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (tobijah@163.com)
//

#include "src/server/kv_server.h"
#include <benchmark/benchmark.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include <utility>

namespace tiny_kv {
std::string GenerateRandomString(size_t length) {
  static const char alphanum[] = "0123456789"
                                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                 "abcdefghijklmnopqrstuvwxyz";
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, sizeof(alphanum) - 2);

  std::string result;
  result.reserve(length);
  for (size_t i = 0; i < length; ++i) {
    result += alphanum[dis(gen)];
  }

  return result;
}

std::vector<std::pair<std::string, std::string>>
GenerateTestData(size_t count, size_t key_size = 10, size_t value_size = 100) {
  std::vector<std::pair<std::string, std::string>> data(count);
  for (size_t i = 0; i < count; ++i) {
    data.emplace_back(GenerateRandomString(key_size),
                      GenerateRandomString(value_size));
  }

  return data;
}

class KVServerBenchmark : public benchmark::Fixture {
public:
  void SetUp(const benchmark::State &state) override {
    server_ = std::make_unique<tiny_kv::KVServer>("127.0.0.1", 8080, "memory");

    test_data_ = GenerateTestData(state.range(0));

    auto &storage = server_->GetStorageForBenchmark();
    for (size_t i = 0; i < test_data_.size() / 2; ++i) {
      for (size_t i = 0; i < test_data_.size(); ++i) {
        storage->Put(test_data_[i].first, test_data_[i].second);
      }
    }
  }

  void TearDown(const benchmark::State &) override { server_.reset(); }

protected:
  std::unique_ptr<tiny_kv::KVServer> server_;
  std::vector<std::pair<std::string, std::string>> test_data_;
};

BENCHMARK_DEFINE_F(KVServerBenchmark, PutBenchmark)(benchmark::State &state) {
  auto &storage = server_->GetStorageForBenchmark();
  size_t i = test_data_.size() / 2;

  for (auto _ : state) {
    if (i >= test_data_.size()) {
      i = test_data_.size() / 2;
    }

    storage->Put(test_data_[i].first, test_data_[i].second);
    ++i;
  }

  state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

BENCHMARK_DEFINE_F(KVServerBenchmark, GetBenchmark)(benchmark::State &state) {
  auto &storage = server_->GetStorageForBenchmark();
  size_t i = 0;

  for (auto _ : state) {
    if (i >= test_data_.size() / 2) {
      i = 0;
    }

    benchmark::DoNotOptimize(storage->Get(test_data_[i].first));
    ++i;
  }

  state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

BENCHMARK_DEFINE_F(KVServerBenchmark, DeleteBenchmark)
(benchmark::State &state) {
  auto &storage = server_->GetStorageForBenchmark();
  size_t i = 0;

  for (auto _ : state) {
    if (i >= test_data_.size() / 2) {
      i = 0;
    }

    storage->Delete(test_data_[i].first);
    ++i;
  }

  state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}

BENCHMARK_REGISTER_F(KVServerBenchmark, PutBenchmark)
    ->Args({1000})
    ->Args({10000});
BENCHMARK_REGISTER_F(KVServerBenchmark, GetBenchmark)
    ->Args({1000})
    ->Args({10000});
BENCHMARK_REGISTER_F(KVServerBenchmark, DeleteBenchmark)
    ->Args({1000})
    ->Args({10000});

} // namespace tiny_kv

BENCHMARK_MAIN();
