// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (tobijah@163.com)
//

#include "kv_server.h"
#include "src/common/kv_common.h"
#include <assert.h>
#include <atomic>
#include <chrono>
#include <csignal>
#include <exception>
#include <future>
#include <gflags/gflags.h>
#include <memory>
#include <signal.h>
#include <thread>

namespace tiny_kv {

DEFINE_string(ip, "127.0.0.1", "The server ip");
DEFINE_int32(port, 8080, "The server port");
DEFINE_string(storage_type, "memory", "Storage type: 'memory' or 'file'");
DEFINE_string(storage_path, "test.db",
              "Path to database file when using file storage");

class KVServerApp;

static KVServerApp *g_app_instance = nullptr;

class KVServerApp {
public:
  KVServerApp() = default;
  ~KVServerApp() = default;

  void SetupSignalHandlers() {
    g_app_instance = this;

    signal(SIGINT, SignalHandlerStatic);
    signal(SIGTERM, SignalHandlerStatic);
  }

  bool Start(const std::string &ip, int port, const std::string &storage_type,
             const std::string &storage_path) {
    server_ = std::make_unique<KVServer>(ip, port, storage_type, storage_path);

    if (!server_->Start()) {
      printf("Failed to start server.\n");
      return false;
    }

    printf("KV Storage Server started, listening on: %s:%d\n", ip.c_str(),
           port);
    printf("Storage engine: %s%s\n", storage_type.c_str(),
           (storage_type == "file" ? (" (path: " + storage_path + ")").c_str()
                                   : ""));
    return true;
  }

  void Run() {
    while (running_) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }

  void Stop() {
    if (shutdown_in_process_) {
      printf("Force exiting...\n");
      exit(1);
    }

    printf("Preparing to exit...\n");
    running_ = false;
    shutdown_in_process_ = true;

    ShutdownServer();
  }

private:
  void ShutdownServer() {
    if (!server_) {
      return;
    }

    try {
      std::promise<void> complete;
      std::future<void> future = complete.get_future();

      std::thread t([this, &complete]() {
        server_->Stop();
        server_.reset();
        complete.set_value();
      });

      printf("Waiting for resources to be released...\n");
      auto status = future.wait_for(std::chrono::seconds(5)); // hard code

      if (status == std::future_status::timeout) {
        printf("Shutdown timeout, forcing exit.\n");
        t.detach();
      } else {
        t.join();
      }
    } catch (const std::exception &e) {
      printf("Exception occurred during shutdown: %s.\n", e.what());
    }

    printf("Server is stopped.\n");
  }

  static void SignalHandlerStatic(int signal) {
    if (g_app_instance) {
      g_app_instance->HandleSignal(signal);
    }
  }

  void HandleSignal(int signal) {
    printf("Received signal %d, preparing to exit...\n", signal);
    Stop();
  }

private:
  std::unique_ptr<KVServer> server_;
  std::atomic<bool> shutdown_in_process_{false};
  std::atomic<bool> running_{true};
};

int main(int argc, char **argv) {

  google::ParseCommandLineFlags(&argc, &argv, true);

  KVServerApp app;

  app.SetupSignalHandlers();

  KV_ASSERT(
      app.Start(FLAGS_ip, FLAGS_port, FLAGS_storage_type, FLAGS_storage_path),
      "Failed to start KV server.");

  app.Run();

  google::ShutDownCommandLineFlags();
  return 0;
}

} // namespace tiny_kv

int main(int argc, char **argv) { return tiny_kv::main(argc, argv); }
