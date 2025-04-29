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
#include <sys/epoll.h>
#include <vector>

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
  std::unique_ptr<StorageEngine> &GetStorageForBenchmark();

private:
  static constexpr int MAX_EVENTS = 1024;
  static constexpr int MAX_BUFFER_SIZE = 1024;

  struct ClientInfo {
    int fd;
    std::string ip;
    int port;
    bool has_address;
    std::vector<char> buffer;
  };

  ClientInfo GetClientInfo(int fd);
  void LogClientEvent(const ClientInfo &client, const std::string &event);
  void HandleClientDisconnect(const ClientInfo &client);
  void ProcessClientRequest(ClientInfo &client, const std::vector<char> &msg);

  void InitHandlers();
  bool InitEpoll();
  void EventLoop();
  void HandleNewConnection();
  bool HandleClientData(int client_fd);
  Request ParseRequest(const std::string &request);
  std::string SerializeResponse(const Response &resp);
  bool SendResponse(int fd, const std::string &response);

private:
  std::string ip_;
  int port_;
  int server_fd_;
  int epoll_fd_;
  std::unique_ptr<StorageEngine> storage_;
  std::atomic<bool> running_;
  std::thread event_thread_;
  std::unordered_map<OperationType, RequestHandle> handlers_;
  std::unordered_map<int, ClientInfo> clients_;
  struct epoll_event events_[MAX_EVENTS];
};
} // namespace tiny_kv
