// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (tobijah@163.com)
//

#include "kv_server.h"
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

namespace tiny_kv {

/************************************************************************/
/* KVServer */
/************************************************************************/
KVServer::KVServer(const std::string &ip, int port,
                   const std::string &storage_type,
                   const std::string &storage_path)
    : ip_(ip), port_(port), server_fd_(-1),
      storage_(CreateStorageEngine(storage_type, storage_path)),
      running_(false) {
  InitHandlers();
}

KVServer::~KVServer() { Stop(); }

bool KVServer::Start() {
  if (running_) {
    return true;
  }

  server_fd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
  if (server_fd_ < 0) {
    return false;
  }

  int opt = 1;
  if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
    close(server_fd_);
    return false;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(ip_.c_str());
  server_addr.sin_port = htons(port_);

  if (bind(server_fd_, (const sockaddr *)&server_addr, sizeof(server_addr)) <
      0) {
    close(server_fd_);
    return false;
  }

  if (listen(server_fd_, 10) < 0) {
    close(server_fd_);
    return false;
  }

  running_ = true;

  accept_thread_ = std::thread(&KVServer::AcceptConnections, this);

  return true;
}

void KVServer::Stop() {
  if (!running_) {
    return;
  }

  running_ = false;

  if (server_fd_ >= 0) {
    close(server_fd_);
    server_fd_ = -1;
  }

  if (accept_thread_.joinable()) {
    accept_thread_.join();
  }

  auto *file_storage = dynamic_cast<FileStorage *>(storage_.get());
  if (file_storage) {
    file_storage->Persist();
  }
}

void KVServer::InitHandlers() {
  handlers_[OperationType::KPut] = [this](const Request &req) -> Response {
    bool success = storage_->Put(req.key, req.value);
    return {success, success ? "success" : "fail", ""};
  };

  handlers_[OperationType::KGet] = [this](const Request &req) -> Response {
    auto value = storage_->Get(req.key);
    if (value.has_value()) {
      return {true, "success", *value};
    } else {
      return {false, "key not found", ""};
    }
  };

  handlers_[OperationType::KDelete] = [this](const Request &req) -> Response {
    bool success = storage_->Delete(req.key);
    return {success, success ? "success" : "fail", ""};
  };
}

void KVServer::AcceptConnections() {
  struct sockaddr_in client_addr;
  socklen_t addr_len = sizeof(client_addr);

  while (running_) {
    int client_fd = accept4(server_fd_, (struct sockaddr *)&client_addr,
                            &addr_len, SOCK_NONBLOCK);

    if (client_fd < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        continue;
      }

      if (!running_) {
        break;
      }

      if (running_) {
        printf("Failed to accept: %s (error: %d)\n", strerror(errno), errno);
      }

      continue;
    }

    std::thread client_thread(&KVServer::HandleClient, this, client_fd);
    client_thread.detach();
  }
}

void KVServer::HandleClient(int client_fd) {
  ClientInfo client = GetClientInfo(client_fd);
  LogClientEvent(client, "connected and ready for requests");

  char buffer[1024] = {0};
  bool session_active = true;

  while (running_ && session_active) {
    memset(buffer, 0, sizeof(buffer));
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_read < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }

    if (bytes_read <= 0) {
      HandleClientDisconnect(client, bytes_read);
      session_active = false;
      continue;
    }

    ProcessClientRequest(client, buffer);
  }

  close(client_fd);
}

KVServer::ClientInfo KVServer::GetClientInfo(int fd) {
  ClientInfo info;
  info.fd = fd;

  struct sockaddr_in addr;
  socklen_t addr_len = sizeof(addr);

  if (getpeername(fd, (struct sockaddr *)&addr, &addr_len) == 0) {
    info.ip = inet_ntoa(addr.sin_addr);
    info.port = ntohs(addr.sin_port);
    info.has_address = true;
  } else {
    info.has_address = false;
  }

  return info;
}

void KVServer::LogClientEvent(const ClientInfo &client,
                              const std::string &event) {
  if (client.has_address) {
    printf("Client %d(%s:%d) %s\n", client.fd, client.ip.c_str(), client.port,
           event.c_str());
  } else {
    printf("Client %d %s\n", client.fd, event.c_str());
  }
  fflush(stdout);
}

void KVServer::HandleClientDisconnect(const ClientInfo &client,
                                      ssize_t status) {
  if (status == 0) {
    LogClientEvent(client, "disconnected normally");
  } else {
    LogClientEvent(client, std::string("connection error: ") + strerror(errno));
  }
}

void KVServer::ProcessClientRequest(const ClientInfo &client,
                                    const char *buffer) {
  Request req = ParseRequest(buffer);

  Response resp;
  auto it = handlers_.find(req.op);
  if (it != handlers_.end()) {
    resp = it->second(req);
  } else {
    resp = {false, "unsupport operation", ""};
  }

  std::string resp_str = SerializeResponse(resp);
  send(client.fd, resp_str.c_str(), resp_str.length(), 0);
}

Request KVServer::ParseRequest(const char *buffer) {
  std::string data(buffer);
  size_t pos = data.find(' ');
  if (pos == std::string::npos) {
    return {OperationType::Invalid, "", ""};
  }

  std::string op_str = data.substr(0, pos);

  static const std::unordered_map<std::string, OperationType> op_map = {
      {"GET", OperationType::KGet},
      {"DEL", OperationType::KDelete},
      {"PUT", OperationType::KPut}};

  auto it = op_map.find(op_str);
  if (it == op_map.end()) {
    return {OperationType::Invalid, "", ""};
  }

  OperationType op = it->second;
  data = data.substr(pos + 1);

  switch (op) {
  case OperationType::KGet:
  case OperationType::KDelete:
    return {op, data, ""};

  case OperationType::KPut: {
    pos = data.find(' ');
    if (pos == std::string::npos) {
      return {op, data, ""};
    }
    return {op, data.substr(0, pos), data.substr(pos + 1)};
  }

  default:
    return {OperationType::Invalid, "", ""};
  }
}

std::string KVServer::SerializeResponse(const Response &resp) {
  std::string result = resp.success ? "SUCCESS" : "FAIL";
  result += " " + resp.message;

  if (!resp.value.empty()) {
    result += " " + resp.value;
  }

  return result;
}

std::unique_ptr<StorageEngine> &KVServer::GetStorageForBenchmark() {
  return storage_;
}

} // namespace tiny_kv
