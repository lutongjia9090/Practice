// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (tobijah@163.com)
//

#include "kv_server.h"
#include "src/common/storage_engine.h"
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <netinet/in.h>
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
    : ip_(ip), port_(port),
      storage_(CreateStorageEngine(storage_type, storage_path)),
      running_(false) {
  InitHandlers();
}

KVServer::~KVServer() { Stop(); }

bool KVServer::Start() {
  if (running_) {
    return true;
  }

  server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
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
  close(server_fd_);

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
    int client_socket =
        accept(server_fd_, (struct sockaddr *)&client_addr, &addr_len);
    if (client_socket < 0) {
      if (running_) {
        printf("Failed to accept: %s (error: %d)\n", strerror(errno), errno);
      }

      continue;
    }

    std::thread client_thread(&KVServer::HandleClient, this, client_socket);
    client_thread.detach();
  }
}

void KVServer::HandleClient(int client_socket) {
  char buffer[1024] = {0};

  while (running_) {
    memset(buffer, 0, sizeof(buffer));

    size_t bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
      break;
    }

    Request req = ParseRequest(buffer);

    Response resp;
    auto it = handlers_.find(req.op);
    if (it != handlers_.end()) {
      resp = it->second(req);
    } else {
      resp = {false, "unsupport operation", ""};
    }

    std::string resp_str = SerializeResponse(resp);
    send(client_socket, resp_str.c_str(), resp_str.length(), 0);
  }

  close(client_socket);
}

Request KVServer::ParseRequest(const char *buffer) {
  std::string data(buffer);
  size_t pos = data.find(' ');
  if (pos == std::string::npos) {
    return {OperationType::Invalid, "", ""};
  }

  OperationType op;
  std::string op_str = data.substr(0, pos);
  if (op_str == "GET") {
    op = OperationType::KGet;
  } else if (op_str == "DEL") {
    op = OperationType::KDelete;
  } else if (op_str == "PUT") {
    op = OperationType::KPut;
  } else {
    return {OperationType::Invalid, "", ""};
  }

  data = data.substr(pos + 1);
  if (op == OperationType::KGet) {
    return {op, data, ""};
  } else if (op == OperationType::KDelete) {
    return {op, data, ""};
  } else {
    pos = data.find(' ');
    if (pos == std::string::npos) {
      return {op, data, ""};
    }

    std::string key = data.substr(0, pos);
    std::string value = data.substr(pos + 1);
    return {op, key, value};
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

} // namespace tiny_kv
