// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (tobijah@163.com)
//

#include "src/grpc_client/grpc_kv_client.h"
#include <atomic>
#include <benchmark/benchmark.h>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <vector>

namespace tiny_kv {

namespace {
const std::string kServerAddress = "127.0.0.1:8080";
} // namespace

/************************************************************************/
/* GenerateRandomString */
/************************************************************************/
std::string GenerateRandomString(size_t length) {
  static const char alphanum[] = "0123456789"
                                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                 "abcdefghijklmnopqrstuvwxyz";

  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<> dis(0, sizeof(alphanum) - 2);

  std::string result;
  result.reserve(length);
  for (size_t i = 0; i < length; ++i) {
    result += alphanum[dis(gen)];
  }

  return result;
}

std::vector<std::pair<std::string, std::string>>
GenerateTestData(size_t count, size_t key_size, size_t value_size) {
  std::vector<std::pair<std::string, std::string>> data;
  data.reserve(count);

  for (size_t i = 0; i < count; ++i) {
    data.emplace_back(GenerateRandomString(key_size),
                      GenerateRandomString(value_size));
  }

  return data;
}

/************************************************************************/
/* ZipfianGenerator */
/************************************************************************/
class ZipfianGenerator {
public:
  ZipfianGenerator(uint64_t min, uint64_t max, double zipfian_const = 0.99)
      : items_(max - min + 1), base_(min), zipfian_const_(zipfian_const),
        gen_(std::random_device{}()) {

    theta_ = zipfian_const_;
    zeta_n_ = CalculateZetaN(items_, theta_);
    alpha_ = 1.0 / (1.0 - theta_);
    eta_ = (1 - std::pow(2.0 / items_, 1 - theta_)) / (1 - zeta_2_ / zeta_n_);
  }

  uint64_t Next() {
    double u = std::uniform_real_distribution<>(0, 1)(gen_);
    double uz = u * zeta_n_;

    if (uz < 1.0) {
      return base_;
    }

    if (uz < 1.0 + std::pow(0.5, theta_)) {
      return base_ + 1;
    }

    return base_ + static_cast<uint64_t>(items_ *
                                         std::pow(eta_ * u - eta_ + 1, alpha_));
  }

private:
  double CalculateZetaN(uint64_t n, double theta) {
    double sum = 0;
    for (uint64_t i = 1; i <= n; i++) {
      sum += 1.0 / std::pow(i, theta);
    }
    return sum;
  }

  const uint64_t items_;
  const uint64_t base_;
  const double zipfian_const_;
  double theta_;
  double alpha_;
  double zeta_n_;
  double eta_;
  static constexpr double zeta_2_ = 1.6449340668482264; // Zeta(2)
  std::mt19937_64 gen_;
};

enum DistributionType { UNIFORM = 0, ZIPFIAN = 1, SEQUENTIAL = 2 };

/************************************************************************/
/* ServerConnectionManager */
/************************************************************************/
class ServerConnectionManager {
public:
  static std::shared_ptr<GrpcKVClient>
  CreateClient(benchmark::State &state, const std::string &server_address) {
    if (!IsServerAvailable(server_address)) {
      state.SkipWithError("The gRPC server is unavailable");
      return nullptr;
    }

    auto client = std::make_shared<GrpcKVClient>(server_address);
    if (!client->Connect()) {
      state.SkipWithError("Failed to connect to gRPC server");
      return nullptr;
    }

    return client;
  }

private:
  static bool CheckConnection(const std::string &server_address) {
    GrpcKVClient client(server_address);
    if (!client.Connect()) {
      fprintf(stderr, "Failed to connect to server: %s - %s\n",
              server_address.c_str(), client.GetLastError().c_str());
      return false;
    }

    if (!client.Put("test_connection_key", "test_connection_value")) {
      fprintf(stderr,
              "Connected but failed to perform test operation on server: %s\n",
              server_address.c_str());
      return false;
    }

    return true;
  }

  static bool IsServerAvailable(const std::string &server_address) {
    static std::once_flag init_flag;
    static bool available = false;

    std::call_once(init_flag, [&server_address]() {
      available = CheckConnection(server_address);
    });

    return available;
  }
};

std::shared_ptr<GrpcKVClient>
CreateClientAndCheckConnection(benchmark::State &state,
                               const std::string &server_address) {
  return ServerConnectionManager::CreateClient(state, server_address);
}

/************************************************************************/
/* BM_GrpcClient_Put */
/************************************************************************/
static void BM_GrpcClient_Put(benchmark::State &state) {
  const size_t data_count = state.range(0);
  const int key_size = state.range(1);
  const int value_size = state.range(2);

  auto client = CreateClientAndCheckConnection(state, kServerAddress);
  if (!client)
    return;

  auto test_data = GenerateTestData(data_count, key_size, value_size);
  size_t success_count = 0;
  size_t failure_count = 0;

  for (auto _ : state) {
    state.PauseTiming();
    size_t i = state.iterations() % data_count;
    state.ResumeTiming();

    bool success = client->Put(test_data[i].first, test_data[i].second);
    if (success) {
      success_count++;
    } else {
      failure_count++;
    }
  }

  state.counters["success_rate"] = benchmark::Counter(
      static_cast<double>(success_count) / state.iterations() * 100.0);
  state.counters["failures"] =
      benchmark::Counter(static_cast<double>(failure_count));

  state.SetItemsProcessed(state.iterations());
  state.SetBytesProcessed(state.iterations() * (key_size + value_size));
}

/************************************************************************/
/* BM_GrpcClient_Get */
/************************************************************************/
static void BM_GrpcClient_Get(benchmark::State &state) {
  const size_t data_count = state.range(0);
  const int key_size = state.range(1);
  const int value_size = state.range(2);

  auto client = CreateClientAndCheckConnection(state, kServerAddress);
  if (!client)
    return;

  auto test_data = GenerateTestData(data_count, key_size, value_size);
  size_t success_count = 0;
  size_t failure_count = 0;
  size_t setup_failures = 0;

  for (const auto &kv : test_data) {
    if (!client->Put(kv.first, kv.second)) {
      setup_failures++;
    }
  }

  if (setup_failures > data_count / 2) {
    state.counters["setup_failures"] =
        benchmark::Counter(static_cast<double>(setup_failures));
  }

  for (auto _ : state) {
    state.PauseTiming();
    size_t i = state.iterations() % data_count;
    state.ResumeTiming();

    auto result = client->Get(test_data[i].first);
    if (result.first) {
      success_count++;
    } else {
      failure_count++;
    }
    benchmark::DoNotOptimize(result);
  }

  state.counters["success_rate"] = benchmark::Counter(
      static_cast<double>(success_count) / state.iterations() * 100.0);
  state.counters["failures"] =
      benchmark::Counter(static_cast<double>(failure_count));

  state.SetItemsProcessed(state.iterations());
  state.SetBytesProcessed(state.iterations() * key_size);
}

/************************************************************************/
/* BM_GrpcClient_MultiGet */
/************************************************************************/
static void BM_GrpcClient_MultiGet(benchmark::State &state) {
  const size_t data_count = state.range(0);
  const int key_size = state.range(1);
  const int value_size = state.range(2);
  const size_t batch_size = state.range(3);

  auto client = CreateClientAndCheckConnection(state, kServerAddress);
  if (!client)
    return;

  auto test_data = GenerateTestData(data_count, key_size, value_size);
  size_t success_count = 0;
  size_t failure_count = 0;
  size_t setup_failures = 0;

  for (const auto &kv : test_data) {
    if (!client->Put(kv.first, kv.second)) {
      setup_failures++;
    }
  }

  if (setup_failures > data_count / 2) {
    state.counters["setup_failures"] =
        benchmark::Counter(static_cast<double>(setup_failures));
  }

  for (auto _ : state) {
    state.PauseTiming();

    std::vector<std::string> keys;
    keys.reserve(batch_size);

    for (size_t i = 0; i < batch_size; ++i) {
      size_t idx = (state.iterations() * batch_size + i) % data_count;
      keys.push_back(test_data[idx].first);
    }

    state.ResumeTiming();

    auto results = client->MultiGet(keys);
    if (results.size() == keys.size()) {
      success_count++;
    } else {
      failure_count++;
    }
    benchmark::DoNotOptimize(results);
  }

  state.counters["success_rate"] = benchmark::Counter(
      static_cast<double>(success_count) / state.iterations() * 100.0);
  state.counters["failures"] =
      benchmark::Counter(static_cast<double>(failure_count));

  state.SetItemsProcessed(state.iterations() * batch_size);
  state.SetBytesProcessed(state.iterations() * batch_size * key_size);
}

/************************************************************************/
/* BM_GrpcService_MixedOperations */
/************************************************************************/
static void BM_GrpcService_MixedOperations(benchmark::State &state) {
  const size_t data_count = state.range(0);
  const int key_size = state.range(1);
  const int value_size = state.range(2);
  const int read_ratio = state.range(3);
  const int dist_type = state.range(4);

  auto client = CreateClientAndCheckConnection(state, kServerAddress);
  if (!client)
    return;

  auto test_data = GenerateTestData(data_count, key_size, value_size);
  size_t read_success = 0;
  size_t read_failure = 0;
  size_t write_success = 0;
  size_t write_failure = 0;
  size_t setup_failures = 0;

  const size_t half_data_count = data_count / 2;
  for (size_t i = 0; i < half_data_count; ++i) {
    if (!client->Put(test_data[i].first, test_data[i].second)) {
      setup_failures++;
    }
  }

  if (setup_failures > half_data_count / 2) {
    state.counters["setup_failures"] =
        benchmark::Counter(static_cast<double>(setup_failures));
  }

  std::unique_ptr<ZipfianGenerator> zipfian;
  std::mt19937 rng(std::random_device{}());
  std::uniform_int_distribution<size_t> uniform_dist(0, data_count - 1);

  if (dist_type == ZIPFIAN) {
    zipfian = std::make_unique<ZipfianGenerator>(0, data_count - 1, 0.99);
  }

  std::bernoulli_distribution read_dist(read_ratio / 100.0);

  for (auto _ : state) {
    state.PauseTiming();
    size_t index;
    switch (dist_type) {
    case UNIFORM:
      index = uniform_dist(rng);
      break;
    case ZIPFIAN:
      index = zipfian->Next();
      break;
    case SEQUENTIAL:
      index = (state.iterations() % data_count);
      break;
    default:
      index = uniform_dist(rng);
    }

    if (read_dist(rng)) {
      auto result = client->Get(test_data[index].first);
      if (result.first) {
        read_success++;
      } else {
        read_failure++;
      }
      benchmark::DoNotOptimize(result);
    } else {
      bool success =
          client->Put(test_data[index].first, test_data[index].second);
      if (success) {
        write_success++;
      } else {
        write_failure++;
      }
    }
  }

  state.counters["read_success_rate"] =
      benchmark::Counter(read_success + read_failure > 0
                             ? static_cast<double>(read_success) /
                                   (read_success + read_failure) * 100.0
                             : 0);
  state.counters["write_success_rate"] =
      benchmark::Counter(write_success + write_failure > 0
                             ? static_cast<double>(write_success) /
                                   (write_success + write_failure) * 100.0
                             : 0);

  state.SetItemsProcessed(state.iterations());
  state.SetBytesProcessed(state.iterations() * (key_size + value_size));
}

/************************************************************************/
/* BM_GrpcService_BatchOperations */
/************************************************************************/
static void BM_GrpcService_BatchOperations(benchmark::State &state) {
  const size_t data_count = state.range(0);
  const int key_size = state.range(1);
  const int value_size = state.range(2);
  const size_t batch_size = state.range(3);

  auto client = CreateClientAndCheckConnection(state, kServerAddress);
  if (!client)
    return;

  auto test_data = GenerateTestData(data_count, key_size, value_size);
  size_t success_count = 0;
  size_t failure_count = 0;
  size_t setup_failures = 0;

  for (const auto &kv : test_data) {
    if (!client->Put(kv.first, kv.second)) {
      setup_failures++;
    }
  }

  if (setup_failures > data_count / 2) {
    state.counters["setup_failures"] =
        benchmark::Counter(static_cast<double>(setup_failures));
  }

  for (auto _ : state) {
    state.PauseTiming();

    std::vector<std::string> keys;
    keys.reserve(batch_size);

    for (size_t i = 0; i < batch_size; ++i) {
      size_t idx = (state.iterations() * batch_size + i) % data_count;
      keys.push_back(test_data[idx].first);
    }

    state.ResumeTiming();

    auto results = client->MultiGet(keys);
    if (results.size() == keys.size()) {
      success_count++;
    } else {
      failure_count++;
    }
    benchmark::DoNotOptimize(results);
  }

  state.counters["success_rate"] = benchmark::Counter(
      static_cast<double>(success_count) / state.iterations() * 100.0);
  state.counters["failures"] =
      benchmark::Counter(static_cast<double>(failure_count));

  state.SetItemsProcessed(state.iterations() * batch_size);
  state.SetBytesProcessed(state.iterations() * batch_size *
                          (key_size + value_size));
}

/************************************************************************/
/* BM_GrpcService_HighLoad */
/************************************************************************/
static void BM_GrpcService_HighLoad(benchmark::State &state) {
  const size_t data_count = state.range(0);
  const int key_size = state.range(1);
  const int value_size = state.range(2);
  const size_t operations_per_iteration = state.range(3);

  auto client = CreateClientAndCheckConnection(state, kServerAddress);
  if (!client)
    return;

  auto test_data = GenerateTestData(data_count, key_size, value_size);
  size_t get_success = 0;
  size_t get_failure = 0;
  size_t put_success = 0;
  size_t put_failure = 0;
  size_t delete_success = 0;
  size_t delete_failure = 0;
  size_t setup_failures = 0;

  const size_t quarter_data_count = data_count / 4;
  for (size_t i = 0; i < quarter_data_count; ++i) {
    if (!client->Put(test_data[i].first, test_data[i].second)) {
      setup_failures++;
    }
  }

  if (setup_failures > quarter_data_count / 2) {
    state.counters["setup_failures"] =
        benchmark::Counter(static_cast<double>(setup_failures));
  }

  std::mt19937 rng(std::random_device{}());
  std::uniform_int_distribution<size_t> index_dist(0, data_count - 1);
  std::uniform_int_distribution<int> op_dist(0, 2); // 0=Get, 1=Put, 2=Delete

  for (auto _ : state) {
    state.PauseTiming();

    std::vector<std::pair<int, size_t>> operations;
    operations.reserve(operations_per_iteration);

    for (size_t i = 0; i < operations_per_iteration; ++i) {
      int op_type = op_dist(rng);
      size_t idx = index_dist(rng);
      operations.emplace_back(op_type, idx);
    }

    state.ResumeTiming();

    for (const auto &op : operations) {
      int op_type = op.first;
      size_t idx = op.second;

      switch (op_type) {
      case 0: // Get
      {
        auto result = client->Get(test_data[idx].first);
        if (result.first) {
          get_success++;
        } else {
          get_failure++;
        }
        benchmark::DoNotOptimize(result);
      } break;
      case 1: // Put
        if (client->Put(test_data[idx].first, test_data[idx].second)) {
          put_success++;
        } else {
          put_failure++;
        }
        break;
      case 2: // Delete
        if (client->Delete(test_data[idx].first)) {
          delete_success++;
        } else {
          delete_failure++;
        }
        break;
      }
    }
  }

  state.counters["get_success_rate"] = benchmark::Counter(
      get_success + get_failure > 0 ? static_cast<double>(get_success) /
                                          (get_success + get_failure) * 100.0
                                    : 0);
  state.counters["put_success_rate"] = benchmark::Counter(
      put_success + put_failure > 0 ? static_cast<double>(put_success) /
                                          (put_success + put_failure) * 100.0
                                    : 0);
  state.counters["delete_success_rate"] =
      benchmark::Counter(delete_success + delete_failure > 0
                             ? static_cast<double>(delete_success) /
                                   (delete_success + delete_failure) * 100.0
                             : 0);

  state.SetItemsProcessed(state.iterations() * operations_per_iteration);
  state.SetBytesProcessed(state.iterations() * operations_per_iteration *
                          (key_size + value_size));
}

/************************************************************************/
/* AsyncCounter */
/************************************************************************/
class AsyncCounter {
public:
  AsyncCounter(size_t expected_count)
      : expected_count_(expected_count), count_(0) {}

  void Increment() {
    std::unique_lock<std::mutex> lock(mutex_);
    ++count_;
    if (count_ >= expected_count_) {
      cv_.notify_all();
    }
  }

  void Wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return count_ >= expected_count_; });
  }

  void Reset() {
    std::unique_lock<std::mutex> lock(mutex_);
    count_ = 0;
  }

private:
  const size_t expected_count_;
  size_t count_;
  std::mutex mutex_;
  std::condition_variable cv_;
};

/************************************************************************/
/* BM_GrpcClient_AsyncPut */
/************************************************************************/
static void BM_GrpcClient_AsyncPut(benchmark::State &state) {
  const size_t data_count = state.range(0);
  const int key_size = state.range(1);
  const int value_size = state.range(2);
  const int concurrency = state.range(3);

  auto client = CreateClientAndCheckConnection(state, kServerAddress);
  if (!client)
    return;

  auto test_data = GenerateTestData(data_count, key_size, value_size);
  std::atomic<size_t> success_count(0);
  std::atomic<size_t> failure_count(0);

  for (auto _ : state) {
    state.PauseTiming();
    AsyncCounter counter(concurrency);
    size_t base_idx = state.iterations() * concurrency;
    state.ResumeTiming();

    for (int i = 0; i < concurrency; ++i) {
      size_t idx = (base_idx + i) % data_count;
      client->AsyncPut(
          test_data[idx].first, test_data[idx].second,
          [&success_count, &failure_count, &counter](bool success) {
            if (success) {
              ++success_count;
            } else {
              ++failure_count;
            }
            counter.Increment();
          });
    }

    counter.Wait();
  }

  state.counters["success_rate"] =
      benchmark::Counter(static_cast<double>(success_count) /
                         (state.iterations() * concurrency) * 100.0);
  state.counters["failures"] =
      benchmark::Counter(static_cast<double>(failure_count));

  state.SetItemsProcessed(state.iterations() * concurrency);
  state.SetBytesProcessed(state.iterations() * concurrency *
                          (key_size + value_size));
}

/************************************************************************/
/* BM_GrpcClient_AsyncGet */
/************************************************************************/
static void BM_GrpcClient_AsyncGet(benchmark::State &state) {
  const size_t data_count = state.range(0);
  const int key_size = state.range(1);
  const int value_size = state.range(2);
  const int concurrency = state.range(3);

  auto client = CreateClientAndCheckConnection(state, kServerAddress);
  if (!client)
    return;

  auto test_data = GenerateTestData(data_count, key_size, value_size);
  std::atomic<size_t> success_count(0);
  std::atomic<size_t> failure_count(0);
  size_t setup_failures = 0;

  for (const auto &kv : test_data) {
    if (!client->Put(kv.first, kv.second)) {
      setup_failures++;
    }
  }

  if (setup_failures > data_count / 2) {
    state.counters["setup_failures"] =
        benchmark::Counter(static_cast<double>(setup_failures));
  }

  for (auto _ : state) {
    state.PauseTiming();
    AsyncCounter counter(concurrency);
    size_t base_idx = state.iterations() * concurrency;
    state.ResumeTiming();

    for (int i = 0; i < concurrency; ++i) {
      size_t idx = (base_idx + i) % data_count;
      client->AsyncGet(test_data[idx].first,
                       [&success_count, &failure_count,
                        &counter](bool success, const std::string &value) {
                         if (success) {
                           ++success_count;
                           benchmark::DoNotOptimize(value);
                         } else {
                           ++failure_count;
                         }
                         counter.Increment();
                       });
    }

    counter.Wait();
  }

  state.counters["success_rate"] =
      benchmark::Counter(static_cast<double>(success_count) /
                         (state.iterations() * concurrency) * 100.0);
  state.counters["failures"] =
      benchmark::Counter(static_cast<double>(failure_count));

  state.SetItemsProcessed(state.iterations() * concurrency);
  state.SetBytesProcessed(state.iterations() * concurrency * key_size);
}

/************************************************************************/
/* BM_GrpcClient_AsyncMultiGet */
/************************************************************************/
static void BM_GrpcClient_AsyncMultiGet(benchmark::State &state) {
  const size_t data_count = state.range(0);
  const int key_size = state.range(1);
  const int value_size = state.range(2);
  const size_t batch_size = state.range(3);
  const int concurrency = state.range(4);

  auto client = CreateClientAndCheckConnection(state, kServerAddress);
  if (!client)
    return;

  auto test_data = GenerateTestData(data_count, key_size, value_size);
  std::atomic<size_t> success_count(0);
  std::atomic<size_t> failure_count(0);
  size_t setup_failures = 0;

  for (const auto &kv : test_data) {
    if (!client->Put(kv.first, kv.second)) {
      setup_failures++;
    }
  }

  if (setup_failures > data_count / 2) {
    state.counters["setup_failures"] =
        benchmark::Counter(static_cast<double>(setup_failures));
  }

  for (auto _ : state) {
    state.PauseTiming();
    AsyncCounter counter(concurrency);
    size_t base_idx = state.iterations() * concurrency * batch_size;
    state.ResumeTiming();

    for (int i = 0; i < concurrency; ++i) {
      std::vector<std::string> keys;
      keys.reserve(batch_size);

      for (size_t j = 0; j < batch_size; ++j) {
        size_t idx = (base_idx + i * batch_size + j) % data_count;
        keys.push_back(test_data[idx].first);
      }

      client->AsyncMultiGet(
          keys,
          [&success_count, &failure_count, &counter, batch_size](
              bool success,
              const std::unordered_map<std::string, std::string> &results) {
            if (success && results.size() == batch_size) {
              ++success_count;
            } else {
              ++failure_count;
            }
            counter.Increment();
          });
    }

    counter.Wait();
  }

  state.counters["success_rate"] =
      benchmark::Counter(static_cast<double>(success_count) /
                         (state.iterations() * concurrency) * 100.0);
  state.counters["failures"] =
      benchmark::Counter(static_cast<double>(failure_count));

  state.SetItemsProcessed(state.iterations() * concurrency * batch_size);
  state.SetBytesProcessed(state.iterations() * concurrency * batch_size *
                          key_size);
}

/************************************************************************/
/* BM_GrpcClient_AsyncMultiPut */
/************************************************************************/
static void BM_GrpcClient_AsyncMultiPut(benchmark::State &state) {
  const size_t data_count = state.range(0);
  const int key_size = state.range(1);
  const int value_size = state.range(2);
  const size_t batch_size = state.range(3);
  const int concurrency = state.range(4);

  auto client = CreateClientAndCheckConnection(state, kServerAddress);
  if (!client)
    return;

  auto test_data = GenerateTestData(data_count, key_size, value_size);
  std::atomic<size_t> success_count(0);
  std::atomic<size_t> failure_count(0);

  for (auto _ : state) {
    state.PauseTiming();
    AsyncCounter counter(concurrency);
    size_t base_idx = state.iterations() * concurrency * batch_size;
    state.ResumeTiming();

    for (int i = 0; i < concurrency; ++i) {
      std::unordered_map<std::string, std::string> kv_pairs;

      for (size_t j = 0; j < batch_size; ++j) {
        size_t idx = (base_idx + i * batch_size + j) % data_count;
        kv_pairs[test_data[idx].first] = test_data[idx].second;
      }

      client->AsyncMultiPut(
          kv_pairs, [&success_count, &failure_count, &counter](bool success) {
            if (success) {
              ++success_count;
            } else {
              ++failure_count;
            }
            counter.Increment();
          });
    }

    counter.Wait();
  }

  state.counters["success_rate"] =
      benchmark::Counter(static_cast<double>(success_count) /
                         (state.iterations() * concurrency) * 100.0);
  state.counters["failures"] =
      benchmark::Counter(static_cast<double>(failure_count));

  state.SetItemsProcessed(state.iterations() * concurrency * batch_size);
  state.SetBytesProcessed(state.iterations() * concurrency * batch_size *
                          (key_size + value_size));
}

/************************************************************************/
/* BM_GrpcService_Async_MixedOperations */
/************************************************************************/
static void BM_GrpcService_Async_MixedOperations(benchmark::State &state) {
  const size_t data_count = state.range(0);
  const int key_size = state.range(1);
  const int value_size = state.range(2);
  const int read_ratio = state.range(3);
  const int dist_type = state.range(4);

  auto client = CreateClientAndCheckConnection(state, kServerAddress);
  if (!client)
    return;

  auto test_data = GenerateTestData(data_count, key_size, value_size);
  std::atomic<size_t> read_success(0);
  std::atomic<size_t> read_failure(0);
  std::atomic<size_t> write_success(0);
  std::atomic<size_t> write_failure(0);
  size_t setup_failures = 0;

  const size_t half_data_count = data_count / 2;
  for (size_t i = 0; i < half_data_count; ++i) {
    if (!client->Put(test_data[i].first, test_data[i].second)) {
      setup_failures++;
    }
  }

  if (setup_failures > half_data_count / 2) {
    state.counters["setup_failures"] =
        benchmark::Counter(static_cast<double>(setup_failures));
  }

  std::unique_ptr<ZipfianGenerator> zipfian;
  std::mt19937 rng(std::random_device{}());
  std::uniform_int_distribution<size_t> uniform_dist(0, data_count - 1);

  if (dist_type == ZIPFIAN) {
    zipfian = std::make_unique<ZipfianGenerator>(0, data_count - 1, 0.99);
  }

  std::bernoulli_distribution read_dist(read_ratio / 100.0);

  for (auto _ : state) {
    state.PauseTiming();
    size_t index;
    switch (dist_type) {
    case UNIFORM:
      index = uniform_dist(rng);
      break;
    case ZIPFIAN:
      index = zipfian->Next();
      break;
    case SEQUENTIAL:
      index = (state.iterations() % data_count);
      break;
    default:
      index = uniform_dist(rng);
    }

    bool is_read = read_dist(rng);
    AsyncCounter counter(1);
    state.ResumeTiming();

    if (is_read) {
      client->AsyncGet(test_data[index].first,
                       [&read_success, &read_failure,
                        &counter](bool success, const std::string &value) {
                         if (success) {
                           ++read_success;
                           benchmark::DoNotOptimize(value);
                         } else {
                           ++read_failure;
                         }
                         counter.Increment();
                       });
    } else {
      client->AsyncPut(
          test_data[index].first, test_data[index].second,
          [&write_success, &write_failure, &counter](bool success) {
            if (success) {
              ++write_success;
            } else {
              ++write_failure;
            }
            counter.Increment();
          });
    }

    counter.Wait();
  }

  state.counters["read_success_rate"] =
      benchmark::Counter(read_success + read_failure > 0
                             ? static_cast<double>(read_success) /
                                   (read_success + read_failure) * 100.0
                             : 0);
  state.counters["write_success_rate"] =
      benchmark::Counter(write_success + write_failure > 0
                             ? static_cast<double>(write_success) /
                                   (write_success + write_failure) * 100.0
                             : 0);

  state.SetItemsProcessed(state.iterations());
  state.SetBytesProcessed(state.iterations() * (key_size + value_size));
}

/************************************************************************/
/* BM_GrpcService_Async_BatchOperations */
/************************************************************************/
static void BM_GrpcService_Async_BatchOperations(benchmark::State &state) {
  const size_t data_count = state.range(0);
  const int key_size = state.range(1);
  const int value_size = state.range(2);
  const size_t batch_size = state.range(3);

  auto client = CreateClientAndCheckConnection(state, kServerAddress);
  if (!client)
    return;

  auto test_data = GenerateTestData(data_count, key_size, value_size);
  std::atomic<size_t> success_count(0);
  std::atomic<size_t> failure_count(0);
  size_t setup_failures = 0;

  for (const auto &kv : test_data) {
    if (!client->Put(kv.first, kv.second)) {
      setup_failures++;
    }
  }

  if (setup_failures > data_count / 2) {
    state.counters["setup_failures"] =
        benchmark::Counter(static_cast<double>(setup_failures));
  }

  for (auto _ : state) {
    state.PauseTiming();

    std::vector<std::string> keys;
    keys.reserve(batch_size);

    for (size_t i = 0; i < batch_size; ++i) {
      size_t idx = (state.iterations() * batch_size + i) % data_count;
      keys.push_back(test_data[idx].first);
    }

    AsyncCounter counter(1);
    state.ResumeTiming();

    client->AsyncMultiGet(
        keys, [&success_count, &failure_count, &counter, &keys](
                  bool success,
                  const std::unordered_map<std::string, std::string> &results) {
          if (success && results.size() == keys.size()) {
            ++success_count;
          } else {
            ++failure_count;
          }
          benchmark::DoNotOptimize(results);
          counter.Increment();
        });

    counter.Wait();
  }

  state.counters["success_rate"] = benchmark::Counter(
      static_cast<double>(success_count) / state.iterations() * 100.0);
  state.counters["failures"] =
      benchmark::Counter(static_cast<double>(failure_count));

  state.SetItemsProcessed(state.iterations() * batch_size);
  state.SetBytesProcessed(state.iterations() * batch_size *
                          (key_size + value_size));
}

/************************************************************************/
/* BM_GrpcService_Async_HighLoad */
/************************************************************************/
static void BM_GrpcService_Async_HighLoad(benchmark::State &state) {
  const size_t data_count = state.range(0);
  const int key_size = state.range(1);
  const int value_size = state.range(2);
  const size_t operations_per_iteration = state.range(3);

  auto client = CreateClientAndCheckConnection(state, kServerAddress);
  if (!client)
    return;

  auto test_data = GenerateTestData(data_count, key_size, value_size);
  std::atomic<size_t> get_success(0);
  std::atomic<size_t> get_failure(0);
  std::atomic<size_t> put_success(0);
  std::atomic<size_t> put_failure(0);
  std::atomic<size_t> delete_success(0);
  std::atomic<size_t> delete_failure(0);
  size_t setup_failures = 0;

  const size_t quarter_data_count = data_count / 4;
  for (size_t i = 0; i < quarter_data_count; ++i) {
    if (!client->Put(test_data[i].first, test_data[i].second)) {
      setup_failures++;
    }
  }

  if (setup_failures > quarter_data_count / 2) {
    state.counters["setup_failures"] =
        benchmark::Counter(static_cast<double>(setup_failures));
  }

  std::mt19937 rng(std::random_device{}());
  std::uniform_int_distribution<size_t> index_dist(0, data_count - 1);
  std::uniform_int_distribution<int> op_dist(0, 2); // 0=Get, 1=Put, 2=Delete

  for (auto _ : state) {
    state.PauseTiming();

    std::vector<std::pair<int, size_t>> operations;
    operations.reserve(operations_per_iteration);

    for (size_t i = 0; i < operations_per_iteration; ++i) {
      int op_type = op_dist(rng);
      size_t idx = index_dist(rng);
      operations.emplace_back(op_type, idx);
    }

    AsyncCounter counter(operations_per_iteration);
    state.ResumeTiming();

    for (const auto &op : operations) {
      int op_type = op.first;
      size_t idx = op.second;

      switch (op_type) {
      case 0: // Get
        client->AsyncGet(test_data[idx].first,
                         [&get_success, &get_failure,
                          &counter](bool success, const std::string &value) {
                           if (success) {
                             ++get_success;
                             benchmark::DoNotOptimize(value);
                           } else {
                             ++get_failure;
                           }
                           counter.Increment();
                         });
        break;
      case 1: // Put
        client->AsyncPut(test_data[idx].first, test_data[idx].second,
                         [&put_success, &put_failure, &counter](bool success) {
                           if (success) {
                             ++put_success;
                           } else {
                             ++put_failure;
                           }
                           counter.Increment();
                         });
        break;
      case 2: // Delete
        client->AsyncDelete(
            test_data[idx].first,
            [&delete_success, &delete_failure, &counter](bool success) {
              if (success) {
                ++delete_success;
              } else {
                ++delete_failure;
              }
              counter.Increment();
            });
        break;
      }
    }

    counter.Wait();
  }

  state.counters["get_success_rate"] = benchmark::Counter(
      get_success + get_failure > 0 ? static_cast<double>(get_success) /
                                          (get_success + get_failure) * 100.0
                                    : 0);
  state.counters["put_success_rate"] = benchmark::Counter(
      put_success + put_failure > 0 ? static_cast<double>(put_success) /
                                          (put_success + put_failure) * 100.0
                                    : 0);
  state.counters["delete_success_rate"] =
      benchmark::Counter(delete_success + delete_failure > 0
                             ? static_cast<double>(delete_success) /
                                   (delete_success + delete_failure) * 100.0
                             : 0);

  state.SetItemsProcessed(state.iterations() * operations_per_iteration);
  state.SetBytesProcessed(state.iterations() * operations_per_iteration *
                          (key_size + value_size));
}

/************************************************************************/
/* Register Benchmarks */
/************************************************************************/
BENCHMARK(BM_GrpcClient_Put)
    // 基础层
    ->Args({1000, 16, 64})
    // 标准层
    ->Args({100000, 16, 64})
    ->Args({100000, 32, 128})
    // 压力层
    ->Args({1000000, 32, 256})
    ->Args({1000000, 64, 1024})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_GrpcClient_Get)
    // 基础层
    ->Args({1000, 16, 64})
    // 标准层
    ->Args({100000, 16, 64})
    ->Args({100000, 32, 128})
    // 压力层
    ->Args({1000000, 32, 256})
    ->Args({1000000, 64, 1024})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_GrpcClient_MultiGet)
    // 基础层 - 小批量，小数据
    ->Args({1000, 16, 64, 10})
    // 标准层 - 中批量，中等数据
    ->Args({100000, 16, 64, 50})
    ->Args({100000, 32, 512, 50})
    // 压力层 - 大批量，大数据
    ->Args({1000000, 32, 1024, 100})
    ->Args({1000000, 32, 1024, 500})
    // 边界测试 - 超大批量
    ->Args({100000, 16, 64, 1000})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_GrpcService_MixedOperations)
    // 基础层 - 不同读写比例
    ->Args({10000, 16, 64, 0, UNIFORM})  // 全写操作
    ->Args({10000, 16, 64, 50, UNIFORM}) // 均衡读写
    ->Args({10000, 16, 64, 95, UNIFORM}) // 读为主
    // 标准层 - 不同访问分布
    ->Args({100000, 16, 64, 75, UNIFORM})    // 均匀分布
    ->Args({100000, 16, 64, 75, ZIPFIAN})    // 热点分布
    ->Args({100000, 16, 64, 75, SEQUENTIAL}) // 顺序访问
    // 压力层 - 大数据集
    ->Args({1000000, 32, 256, 75, UNIFORM})
    ->Args({1000000, 32, 256, 75, ZIPFIAN})
    // 多种值大小
    ->Args({100000, 16, 2048, 75, ZIPFIAN}) // 中等值大小
    ->Args({100000, 16, 8192, 75, ZIPFIAN}) // 大值大小
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_GrpcService_BatchOperations)
    // 基础层 - 小批量
    ->Args({10000, 16, 64, 10})
    // 标准层 - 中批量
    ->Args({100000, 16, 64, 50})
    ->Args({100000, 16, 512, 50})
    // 压力层 - 大批量
    ->Args({1000000, 16, 64, 100})
    ->Args({100000, 16, 64, 500})
    // 边界测试 - 超大批量
    ->Args({100000, 16, 64, 2000})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_GrpcService_HighLoad)
    // 基础层 - 低负载
    ->Args({10000, 16, 64, 20})
    // 标准层 - 中负载
    ->Args({100000, 16, 64, 100})
    ->Args({100000, 16, 512, 100})
    // 压力层 - 高负载
    ->Args({1000000, 16, 64, 500})
    ->Args({100000, 16, 64, 1000})
    // 极限负载
    ->Args({100000, 16, 64, 5000})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_GrpcClient_AsyncPut)
    // 基础层 - 低并发
    ->Args({10000, 16, 64, 20})
    // 标准层 - 中等并发
    ->Args({100000, 16, 64, 100})
    ->Args({100000, 16, 512, 100})
    // 压力层 - 高并发
    ->Args({1000000, 16, 64, 500})
    ->Args({100000, 16, 64, 1000})
    // 极限并发
    ->Args({100000, 16, 64, 2000})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_GrpcClient_AsyncGet)
    // 基础层 - 低并发
    ->Args({10000, 16, 64, 20})
    // 标准层 - 中等并发
    ->Args({100000, 16, 64, 100})
    ->Args({100000, 16, 512, 100})
    // 压力层 - 高并发
    ->Args({1000000, 16, 64, 500})
    ->Args({100000, 16, 64, 1000})
    // 极限并发
    ->Args({100000, 16, 64, 2000})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_GrpcClient_AsyncMultiGet)
    // 基础层 - 小批量和低并发
    ->Args({10000, 16, 64, 10, 10})
    // 标准层 - 中批量和中等并发
    ->Args({100000, 16, 64, 50, 50})
    ->Args({100000, 16, 512, 50, 50})
    // 压力层 - 大批量或高并发
    ->Args({1000000, 16, 64, 50, 200})
    ->Args({100000, 16, 64, 200, 50})
    // 极限测试 - 超大批量和高并发
    ->Args({100000, 16, 64, 100, 500})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_GrpcClient_AsyncMultiPut)
    // 基础层 - 小批量和低并发
    ->Args({10000, 16, 64, 10, 10})
    // 标准层 - 中批量和中等并发
    ->Args({100000, 16, 64, 50, 50})
    ->Args({100000, 16, 512, 50, 50})
    // 压力层 - 大批量或高并发
    ->Args({1000000, 16, 64, 50, 200})
    ->Args({100000, 16, 64, 200, 50})
    // 极限测试 - 超大批量和高并发
    ->Args({100000, 16, 64, 100, 500})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_GrpcService_Async_MixedOperations)
    // 基础层 - 不同读写比例
    ->Args({10000, 16, 64, 0, UNIFORM})  // 全写操作
    ->Args({10000, 16, 64, 50, UNIFORM}) // 均衡读写
    ->Args({10000, 16, 64, 95, UNIFORM}) // 读为主
    // 标准层 - 不同访问分布
    ->Args({100000, 16, 64, 75, UNIFORM})    // 均匀分布
    ->Args({100000, 16, 64, 75, ZIPFIAN})    // 热点分布
    ->Args({100000, 16, 64, 75, SEQUENTIAL}) // 顺序访问
    // 压力层 - 大数据集
    ->Args({1000000, 32, 256, 75, UNIFORM})
    ->Args({1000000, 32, 256, 75, ZIPFIAN})
    // 多种值大小
    ->Args({100000, 16, 2048, 75, ZIPFIAN}) // 中等值大小
    ->Args({100000, 16, 8192, 75, ZIPFIAN}) // 大值大小
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_GrpcService_Async_BatchOperations)
    // 基础层 - 小批量
    ->Args({10000, 16, 64, 10})
    // 标准层 - 中批量
    ->Args({100000, 16, 64, 50})
    ->Args({100000, 16, 512, 50})
    // 压力层 - 大批量
    ->Args({1000000, 16, 64, 100})
    ->Args({100000, 16, 64, 500})
    // 边界测试 - 超大批量
    ->Args({100000, 16, 64, 2000})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_GrpcService_Async_HighLoad)
    // 基础层 - 低负载
    ->Args({10000, 16, 64, 20})
    // 标准层 - 中负载
    ->Args({100000, 16, 64, 100})
    ->Args({100000, 16, 512, 100})
    // 压力层 - 高负载
    ->Args({1000000, 16, 64, 500})
    ->Args({100000, 16, 64, 1000})
    // 极限负载
    ->Args({100000, 16, 64, 5000})
    ->Unit(benchmark::kMicrosecond);

} // namespace tiny_kv

BENCHMARK_MAIN();
