// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (tobijah@163.com)
//

#pragma once

#include "src/proto/kv_service.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <unordered_map>

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

  std::unordered_map<std::string, std::string> MultiGet(const std::vector<std::string> &keys);
  bool MultiPut(const std::unordered_map<std::string, std::string> &kv_pairs);
  bool MultiDelete(const std::vector<std::string> &keys);

  std::string GetLastError() const;

private:
  std::string server_address_;
  std::string last_error_;
  std::shared_ptr<grpc::Channel> channel_;
  std::unique_ptr<KVService::Stub> stub_;
  bool connected_;
};

} // namespace tiny_kv
