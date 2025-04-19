// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (tobijah@163.com)
//

#include "src/client/kv_client.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <gflags/gflags.h>
#include <mutex>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace tiny_kv {

DEFINE_string(server_ip, "127.0.0.1", "server ip");
DEFINE_int32(server_port, 8080, "server port");
DEFINE_int32(client_threads, 1, "number of concurrent client threads");
DEFINE_int32(test_duration, 5, "test duration in seconds");
DEFINE_int32(key_size, 10, "key size in bytes");
DEFINE_int32(value_size, 100, "value size in bytes");
DEFINE_int32(data_count, 10000, "number of test data items");

#define SMART_BENCHMARK_DO_NOT_OPTIMIZE(x)                                     \
  __asm__ __volatile__("" ::"g"(x) : "memory")

enum StatusCode { OK = 0, ERROR = 1, MAX_STATUS_CODE = 2 };

enum OpType { PUT = 0, GET = 1, DELETE = 2, MAX_OP_TYPE = 3 };

OpType GetOpTypeEnum(const std::string &op_type) {
  if (op_type == "put")
    return PUT;
  if (op_type == "get")
    return GET;
  if (op_type == "delete")
    return DELETE;
  return MAX_OP_TYPE;
}

const char *GetOpTypeStr(OpType op_type) {
  static const char *OP_TYPE_STR[MAX_OP_TYPE] = {"PUT", "GET", "DELETE"};
  if (op_type < MAX_OP_TYPE) {
    return OP_TYPE_STR[op_type];
  }
  return "UNKNOWN";
}

const char *GetStatusCodeStr(StatusCode code) {
  static const char *STATUS_CODE_STR[MAX_STATUS_CODE] = {"OK", "ERROR"};
  if (code < MAX_STATUS_CODE) {
    return STATUS_CODE_STR[code];
  }
  return "UNKNOWN";
}

void CheckArguments() {
  if (FLAGS_client_threads <= 0) {
    fprintf(stderr, "Number of client threads must be > 0\n");
    exit(1);
  }
  if (FLAGS_test_duration <= 0) {
    fprintf(stderr, "Test duration must be > 0\n");
    exit(1);
  }
  if (FLAGS_key_size <= 0 || FLAGS_value_size <= 0) {
    fprintf(stderr, "Key and value sizes must be > 0\n");
    exit(1);
  }
  if (FLAGS_data_count <= 0) {
    fprintf(stderr, "Test data count must be > 0\n");
    exit(1);
  }
}

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
  std::vector<std::pair<std::string, std::string>> data;
  for (size_t i = 0; i < count; ++i) {
    data.emplace_back(GenerateRandomString(key_size),
                      GenerateRandomString(value_size));
  }

  return data;
}

class BenchmarkStat {
public:
  BenchmarkStat() { clear(); }

  void clear() {
    latencies_.clear();
    for (size_t i = 0; i < MAX_STATUS_CODE; ++i) {
      status_code_counts_[i] = 0;
    }
    time_cost_ = 0;
    time_cost_weight_ = 1.0;
  }

  void reserve(size_t n) { latencies_.reserve(n); }

  void AddLatency(double latency, StatusCode status) {
    latencies_.push_back(latency);
    status_code_counts_[status]++;
  }

  void SetTimeCost(double seconds) {
    time_cost_ = seconds;
    time_cost_weight_ = 1.0;
  }

  void Merge(const BenchmarkStat &other) {
    latencies_.insert(latencies_.end(), other.latencies_.begin(),
                      other.latencies_.end());
    for (size_t i = 0; i < MAX_STATUS_CODE; ++i) {
      status_code_counts_[i] += other.status_code_counts_[i];
    }
    time_cost_ += other.time_cost_;
    time_cost_weight_ += other.time_cost_weight_;
  }

  void PrepareDump() { std::sort(latencies_.begin(), latencies_.end()); }

  void Dump(OpType op_type) const {
    if (latencies_.empty()) {
      printf("No statistics collected\n");
      return;
    }

    size_t queries = latencies_.size();

    printf("\n============= %s Operation Statistics =============\n",
           GetOpTypeStr(op_type));

    // Status code statistics
    printf("%-16s %-16s %-16s\n", "Status", "Count", "Percentage (%)");
    for (size_t i = 0; i < MAX_STATUS_CODE; ++i) {
      if (status_code_counts_[i] > 0) {
        printf("%-16s %-16zu %-16.2f\n",
               GetStatusCodeStr(static_cast<StatusCode>(i)),
               status_code_counts_[i],
               100.0 * status_code_counts_[i] / queries);
      }
    }
    printf("\n");

    // Latency statistics
    double mean_latency =
        std::accumulate(latencies_.begin(), latencies_.end(), 0.0) / queries;
    auto percentile_latency = [this, queries](double percentile) {
      return latencies_[(size_t)floor(queries * percentile)];
    };

    printf("%-16s %-16s\n", "Latency", "Milliseconds");
    printf("%-16s %-16.3f\n", "Average", mean_latency);
    printf("%-16s %-16.3f\n", "50%", percentile_latency(0.5));
    printf("%-16s %-16.3f\n", "90%", percentile_latency(0.9));
    printf("%-16s %-16.3f\n", "95%", percentile_latency(0.95));
    printf("%-16s %-16.3f\n", "99%", percentile_latency(0.99));
    printf("%-16s %-16.3f\n", "99.9%", percentile_latency(0.999));
    printf("%-16s %-16.3f\n", "Max", latencies_.back());
    printf("\n");

    // QPS statistics
    double seconds = time_cost_ / time_cost_weight_;
    double qps = queries / seconds;
    printf("%-16s %-16s %-16s\n", "Total Requests", "Total Time (s)", "QPS");
    printf("%-16zu %-16.3f %-16.3f\n", queries, seconds, qps);
    printf("\n");
  }

private:
  std::vector<double> latencies_;
  size_t status_code_counts_[MAX_STATUS_CODE] = {0};
  double time_cost_ = 0;
  double time_cost_weight_ = 1.0;
};

std::vector<std::pair<std::string, std::string>> g_test_data;
std::atomic<bool> g_running{false};
std::atomic<size_t> g_operations_completed{0};
std::mutex g_print_mutex;

void SetupGlobalTestData() {
  g_test_data =
      GenerateTestData(FLAGS_data_count, FLAGS_key_size, FLAGS_value_size);

  printf("Connecting to server %s:%d for data preloading...\n",
         FLAGS_server_ip.c_str(), FLAGS_server_port);

  KVClient preload_client(FLAGS_server_ip, FLAGS_server_port);
  if (!preload_client.Connect()) {
    fprintf(stderr, "Failed to connect to server for data preloading: %s\n",
            preload_client.GetLastError().c_str());
    exit(1);
  }

  for (size_t i = 0; i < g_test_data.size() / 2; ++i) {
    if (i % 1000 == 0 && i > 0) {
      printf("Preloaded [%zu/%zu] records...\n", i, g_test_data.size() / 2);
    }
    preload_client.Put(g_test_data[i].first, g_test_data[i].second);
  }
  printf("Preloading completed, loaded %zu records\n", g_test_data.size() / 2);
}

void ClientWorker(
    OpType op_type, BenchmarkStat *thread_stats,
    std::chrono::time_point<std::chrono::steady_clock> start_time) {
  KVClient client(FLAGS_server_ip, FLAGS_server_port);
  if (!client.Connect()) {
    return;
  }

  size_t i = 0;

  while (g_running) {
    bool success = false;
    auto op_start_time = std::chrono::steady_clock::now();

    switch (op_type) {
    case PUT: {
      // Use the second half of data for Put testing
      i = (g_test_data.size() / 2) + (i % (g_test_data.size() / 2));
      success = client.Put(g_test_data[i].first, g_test_data[i].second);
      break;
    }
    case GET: {
      // Use the first half of data for Get testing (preloaded)
      i = i % (g_test_data.size() / 2);
      auto[ok, value] = client.Get(g_test_data[i].first);
      SMART_BENCHMARK_DO_NOT_OPTIMIZE(value);
      success = ok;
      break;
    }
    case DELETE: {
      // Use the first half of data for Delete testing (preloaded)
      i = i % (g_test_data.size() / 2);
      success = client.Delete(g_test_data[i].first);
      break;
    }
    default:
      break;
    }

    auto op_end_time = std::chrono::steady_clock::now();
    double latency =
        std::chrono::duration<double, std::milli>(op_end_time - op_start_time)
            .count();

    thread_stats->AddLatency(latency, success ? OK : ERROR);

    g_operations_completed++;
    i++;
  }

  auto end_time = std::chrono::steady_clock::now();
  double thread_seconds =
      std::chrono::duration<double>(end_time - start_time).count();
  thread_stats->SetTimeCost(thread_seconds);
}

void RunBenchmark(const std::string &op_type_str) {
  OpType op_type = GetOpTypeEnum(op_type_str);
  g_operations_completed = 0;
  g_running = true;

  printf("\nStarting %s benchmark with %d threads, running for %d seconds...\n",
         GetOpTypeStr(op_type), FLAGS_client_threads, FLAGS_test_duration);

  auto start_time = std::chrono::steady_clock::now();

  std::vector<BenchmarkStat> thread_stats(FLAGS_client_threads);
  std::vector<std::thread> threads;

  for (int i = 0; i < FLAGS_client_threads; i++) {
    thread_stats[i].reserve(FLAGS_test_duration * 1000);
    threads.emplace_back(ClientWorker, op_type, &thread_stats[i], start_time);
  }

  std::this_thread::sleep_for(std::chrono::seconds(FLAGS_test_duration));
  g_running = false;

  for (auto &thread : threads) {
    thread.join();
  }

  BenchmarkStat final_stats;
  for (int i = 0; i < FLAGS_client_threads; i++) {
    final_stats.Merge(thread_stats[i]);
  }

  final_stats.PrepareDump();

  auto end_time = std::chrono::steady_clock::now();
  double total_seconds =
      std::chrono::duration<double>(end_time - start_time).count();
  final_stats.SetTimeCost(total_seconds);

  final_stats.Dump(op_type);
}

int main(int argc, char **argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  CheckArguments();

  SetupGlobalTestData();

  RunBenchmark("put");
  RunBenchmark("get");
  RunBenchmark("delete");

  google::ShutDownCommandLineFlags();
  return 0;
}

} // namespace tiny_kv

int main(int argc, char **argv) { return tiny_kv::main(argc, argv); }
