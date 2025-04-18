// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (tobijah@163.com)
//

#pragma once
#include "src/common/kv_common.h"
#include "src/common/storage_engine.h"
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>

namespace tiny_kv {

using RequestHandle = std::function<Response(const Request &)>;

/************************************************************************/
/* KVServer */
/************************************************************************/
class KVServer {
public:
  KVServer(const std::string &ip, int port,
           const std::string &storage_type = "memory",
           const std::string &storage_path = "");

  ~KVServer();

  bool Start();
  void Stop();

private:
  struct ClientInfo {
    int socket;
    std::string ip;
    int port;
    bool has_address;
  };

  ClientInfo GetClientInfo(int socket);
  void LogClientEvent(const ClientInfo &client, const std::string &event);
  void HandleClientDisconnect(const ClientInfo &client, ssize_t status);
  void ProcessClientRequest(const ClientInfo &client, const char *buffer);

  void InitHandlers();
  void AcceptConnections();
  void HandleClient(int client_socket);
  Request ParseRequest(const char *buffer);
  std::string SerializeResponse(const Response &resp);

private:
  std::string ip_;
  int port_;
  int server_fd_;
  std::unique_ptr<StorageEngine> storage_;
  std::atomic<bool> running_;
  std::thread accept_thread_;
  std::unordered_map<OperationType, RequestHandle> handlers_;
};
} // namespace tiny_kv
