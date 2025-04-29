// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (tobijah@163.com)
//

#include "kv_server.h"
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>

namespace tiny_kv {

/************************************************************************/
/* KVServer */
/************************************************************************/
KVServer::KVServer(const std::string &ip, int port,
                   const std::string &storage_type,
                   const std::string &storage_path)
    : ip_(ip), port_(port), server_fd_(-1), epoll_fd_(-1),
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

  if (listen(server_fd_, SOMAXCONN) < 0) {
    close(server_fd_);
    return false;
  }

  if (!InitEpoll()) {
    close(server_fd_);
    return false;
  }

  running_ = true;
  event_thread_ = std::thread(&KVServer::EventLoop, this);

  return true;
}

void KVServer::Stop() {
  if (!running_) {
    return;
  }

  running_ = false;

  if (event_thread_.joinable()) {
    event_thread_.join();
  }

  for (const auto &client : clients_) {
    close(client.first);
  }
  clients_.clear();

  if (epoll_fd_ >= 0) {
    close(epoll_fd_);
    epoll_fd_ = -1;
  }

  if (server_fd_ >= 0) {
    close(server_fd_);
    server_fd_ = -1;
  }

  auto *file_storage = dynamic_cast<FileStorage *>(storage_.get());
  if (file_storage) {
    file_storage->Persist();
  }
}

bool KVServer::InitEpoll() {
  epoll_fd_ = epoll_create1(0);
  if (epoll_fd_ < 0) {
    return false;
  }

  struct epoll_event event;
  event.events = EPOLLIN | EPOLLET;
  event.data.fd = server_fd_;

  if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, server_fd_, &event) < 0) {
    close(epoll_fd_);
    return false;
  }

  return true;
}

void KVServer::EventLoop() {
  while (running_) {
    int nfds = epoll_wait(epoll_fd_, events_, MAX_EVENTS, 100);
    if (nfds < 0) {
      if (errno == EINTR) {
        continue;
      }
      break;
    }

    for (int i = 0; i < nfds; i++) {
      if (events_[i].data.fd == server_fd_) {
        HandleNewConnection();
      } else {
        if (!HandleClientData(events_[i].data.fd)) {
          HandleClientDisconnect(clients_[events_[i].data.fd]);
          clients_.erase(events_[i].data.fd);
        }
      }
    }
  }
}

void KVServer::HandleNewConnection() {
  while (running_) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    int client_fd = accept4(server_fd_, (struct sockaddr *)&client_addr,
                            &addr_len, SOCK_NONBLOCK);

    if (client_fd < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      }
      continue;
    }

    struct epoll_event event;
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = client_fd;

    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &event) < 0) {
      close(client_fd);
      continue;
    }

    ClientInfo client_info = GetClientInfo(client_fd);
    clients_[client_fd] = std::move(client_info);
    LogClientEvent(clients_[client_fd], "connected");
  }
}

bool KVServer::HandleClientData(int client_fd) {
  auto &client = clients_[client_fd];
  char buf[MAX_BUFFER_SIZE];

  while (true) {
    ssize_t n = read(client_fd, buf, sizeof(buf));
    if (n < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      }
      return false;
    }

    if (n == 0) {
      return false;
    }

    client.buffer.insert(client.buffer.end(), buf, buf + n);

    auto it = std::search(client.buffer.begin(), client.buffer.end(),
                          std::begin("\r\n"), std::end("\r\n") - 1);

    while (it != client.buffer.end()) {
      std::vector<char> msg(client.buffer.begin(), it);
      ProcessClientRequest(client, msg);

      client.buffer.erase(client.buffer.begin(), it + 2);

      it = std::search(client.buffer.begin(), client.buffer.end(),
                       std::begin("\r\n"), std::end("\r\n") - 1);
    }
  }

  return true;
}

void KVServer::ProcessClientRequest(ClientInfo &client,
                                    const std::vector<char> &msg) {
  std::string request(msg.begin(), msg.end());
  Request req = ParseRequest(request);
  auto it = handlers_.find(req.op);
  if (it == handlers_.end()) {
    Response resp{false, "unknown operation", "", {}};
    SendResponse(client.fd, SerializeResponse(resp) + "\r\n");
    return;
  }
  Response resp = it->second(req);
  SendResponse(client.fd, SerializeResponse(resp) + "\r\n");
}

bool KVServer::SendResponse(int fd, const std::string &response) {
  size_t total_sent = 0;
  while (total_sent < response.size()) {
    ssize_t sent =
        write(fd, response.c_str() + total_sent, response.size() - total_sent);
    if (sent < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        continue;
      }
      return false;
    }
    total_sent += sent;
  }
  return true;
}

void KVServer::InitHandlers() {
  handlers_[OperationType::KPut] = [this](const Request &req) -> Response {
    bool success = storage_->Put(req.key, req.value);
    return {success, success ? "success" : "fail", "", {}};
  };

  handlers_[OperationType::KGet] = [this](const Request &req) -> Response {
    auto value = storage_->Get(req.key);
    if (value.has_value()) {
      return {true, "success", *value, {}};
    } else {
      return {false, "key not found", "", {}};
    }
  };

  handlers_[OperationType::KDelete] = [this](const Request &req) -> Response {
    bool success = storage_->Delete(req.key);
    return {success, success ? "success" : "fail", "", {}};
  };

  handlers_[OperationType::KMultiGet] = [this](const Request &req) -> Response {
    Response resp{true, "success", "", {}};
    resp.kvs.reserve(req.kvs.size());

    for (const auto &kv : req.kvs) {
      auto value = storage_->Get(kv.key);
      if (value.has_value()) {
        resp.kvs.push_back({kv.key, *value});
      } else {
        resp.kvs.push_back({kv.key, ""});
      }
    }

    return resp;
  };

  handlers_[OperationType::KMultiPut] = [this](const Request &req) -> Response {
    bool success = true;

    for (const auto &kv : req.kvs) {
      if (!storage_->Put(kv.key, kv.value)) {
        success = false;
      }
    }

    return {success, success ? "success" : "fail", "", {}};
  };

  handlers_[OperationType::KMultiDelete] =
      [this](const Request &req) -> Response {
    bool success = true;

    for (const auto &kv : req.kvs) {
      if (!storage_->Delete(kv.key)) {
        success = false;
      }
    }

    return {success, success ? "success" : "fail", "", {}};
  };
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

void KVServer::HandleClientDisconnect(const ClientInfo &client) {
  LogClientEvent(client, "disconnected");
  epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client.fd, nullptr);
  close(client.fd);
}

Request KVServer::ParseRequest(const std::string &request) {
  std::string data = request;
  size_t pos = data.find(' ');
  if (pos == std::string::npos) {
    return {OperationType::Invalid, "", "", {}};
  }

  std::string op_str = data.substr(0, pos);

  static const std::unordered_map<std::string, OperationType> op_map = {
      {"GET", OperationType::KGet},
      {"DEL", OperationType::KDelete},
      {"PUT", OperationType::KPut},
      {"MGET", OperationType::KMultiGet},
      {"MPUT", OperationType::KMultiPut},
      {"MDEL", OperationType::KMultiDelete}};

  auto it = op_map.find(op_str);
  if (it == op_map.end()) {
    return {OperationType::Invalid, "", "", {}};
  }

  OperationType op = it->second;
  data = data.substr(pos + 1);

  switch (op) {
  case OperationType::KGet:
  case OperationType::KDelete:
    return {op, data, "", {}};

  case OperationType::KPut: {
    pos = data.find(' ');
    if (pos == std::string::npos) {
      return {op, data, "", {}};
    }
    return {op, data.substr(0, pos), data.substr(pos + 1), {}};
  }

  case OperationType::KMultiGet:
  case OperationType::KMultiDelete: {
    Request req{op, "", "", {}};
    size_t start = 0;
    while (start < data.length()) {
      pos = data.find(' ', start);
      std::string key;
      if (pos == std::string::npos) {
        key = data.substr(start);
        start = data.length();
      } else {
        key = data.substr(start, pos - start);
        start = pos + 1;
      }
      if (!key.empty()) {
        req.kvs.push_back({key, ""});
      }
    }
    return req;
  }

  case OperationType::KMultiPut: {
    Request req{op, "", "", {}};
    size_t start = 0;
    while (start < data.length()) {
      size_t key_end = data.find(' ', start);
      if (key_end == std::string::npos) {
        break;
      }
      std::string key = data.substr(start, key_end - start);
      start = key_end + 1;

      size_t value_end = data.find(' ', start);
      std::string value;
      if (value_end == std::string::npos) {
        value = data.substr(start);
        start = data.length();
      } else {
        value = data.substr(start, value_end - start);
        start = value_end + 1;
      }

      if (!key.empty()) {
        req.kvs.push_back({key, value});
      }
    }
    return req;
  }

  default:
    return {OperationType::Invalid, "", "", {}};
  }
}

std::string KVServer::SerializeResponse(const Response &resp) {
  std::string result = resp.success ? "SUCCESS" : "FAIL";
  result += " " + resp.message;

  if (!resp.value.empty()) {
    result += " " + resp.value;
  }

  if (!resp.kvs.empty()) {
    for (const auto &kv : resp.kvs) {
      result += " " + kv.key + " " + kv.value;
    }
  }

  return result;
}

std::unique_ptr<StorageEngine> &KVServer::GetStorageForBenchmark() {
  return storage_;
}

} // namespace tiny_kv
