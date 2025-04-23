// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (tobijah@163.com)
//

#pragma once

#include "src/proto/kv_service.grpc.pb.h"
#include <atomic>
#include <functional>
#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace tiny_kv {

/************************************************************************/
/* GrpcKVClient */
/************************************************************************/
class GrpcKVClient {
public:
  GrpcKVClient(const std::string &server_address);
  ~GrpcKVClient();

  bool Connect();

  void Shutdown();

  std::pair<bool, std::string> Get(const std::string &key);
  bool Put(const std::string &key, const std::string &value);
  bool Delete(const std::string &key);
  std::unordered_map<std::string, std::string>
  MultiGet(const std::vector<std::string> &keys);
  bool MultiPut(const std::unordered_map<std::string, std::string> &kv_pairs);
  bool MultiDelete(const std::vector<std::string> &keys);

  std::string GetLastError() const;

  void AsyncGet(const std::string &key,
                std::function<void(bool, const std::string &)> callback);
  void AsyncPut(const std::string &key, const std::string &value,
                std::function<void(bool)> callback);
  void AsyncDelete(const std::string &key, std::function<void(bool)> callback);
  void AsyncMultiGet(
      const std::vector<std::string> &keys,
      std::function<void(bool,
                         const std::unordered_map<std::string, std::string> &)>
          callback);
  void
  AsyncMultiPut(const std::unordered_map<std::string, std::string> &kv_pairs,
                std::function<void(bool)> callback);
  void AsyncMultiDelete(const std::vector<std::string> &keys,
                        std::function<void(bool)> callback);

private:
  std::string server_address_;
  std::string last_error_;
  std::shared_ptr<grpc::Channel> channel_;
  std::unique_ptr<KVService::Stub> stub_;
  bool connected_;

  grpc::CompletionQueue cq_;
  std::atomic<bool> running_{false};
  std::thread completion_thread_;
};

} // namespace tiny_kv
