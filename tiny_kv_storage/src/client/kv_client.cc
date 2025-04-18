// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (tobijah@163.com)
//

#include "kv_client.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace tiny_kv {

/************************************************************************/
/* KVClient */
/************************************************************************/
KVClient::KVClient(const std::string &server_ip, int server_port)
    : server_ip_(server_ip), server_port_(server_port), connected_(false) {}

KVClient::~KVClient() { Disconnect(); }

bool KVClient::Connect() {
  if (connected_) {
    return true;
  }

  socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd_ < 0) {
    last_error_ = "Failed to create socket.";
    return false;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(server_port_);

  if (inet_pton(AF_INET, server_ip_.c_str(), &server_addr.sin_addr) <= 0) {
    last_error_ = "Invalid server address.";
    close(socket_fd_);
    return false;
  }

  if (connect(socket_fd_, (struct sockaddr *)&server_addr,
              sizeof(server_addr)) < 0) {
    last_error_ = "Failed to connect server.";
    close(socket_fd_);
    return false;
  }

  connected_ = true;
  return true;
}

void KVClient::Disconnect() {
  if (connected_) {
    close(socket_fd_);
    connected_ = false;
  }
}

std::pair<bool, std::string> KVClient::Get(const std::string &key) {
  return ExecuteCmd("GET", key);
}

bool KVClient::Put(const std::string &key, const std::string &value) {
  auto[success, message] = ExecuteCmd("PUT", key, value);
  return success;
}

bool KVClient::Delete(const std::string &key) {
  auto[success, message] = ExecuteCmd("DEL", key);
  return success;
}

std::string KVClient::GetLastError() const { return last_error_; }

bool KVClient::EnsureConnect() {
  if (!connected_) {
    return Connect();
  }

  return true;
}

bool KVClient::SendRequest(const std::string &request) {
  ssize_t bytes_sent = send(socket_fd_, request.c_str(), request.length(), 0);
  if (bytes_sent != static_cast<ssize_t>(request.length())) {
    last_error_ = "Failed to send request.";
    return false;
  }

  return true;
}

bool KVClient::ReceiveResponse(std::string &response) {
  char buffer[1024] = {0};
  ssize_t bytes_read = recv(socket_fd_, buffer, sizeof(buffer) - 1, 0);
  if (bytes_read <= 0) {
    last_error_ = "Failed to receive response.";
    return false;
  }

  response = std::string(buffer, bytes_read);
  return true;
}

std::pair<bool, std::string>
KVClient::ParseResponse(const std::string &response) {
  size_t pos = response.find(' ');
  if (pos == std::string::npos) {
    return {false, "Invalid response."};
  }

  std::string status = response.substr(0, pos);
  bool success = (status == "SUCCESS");

  std::string rest = response.substr(pos + 1);
  pos = rest.find(' ');
  if (success && pos != std::string::npos) {
    std::string message = rest.substr(0, pos);
    std::string value = rest.substr(pos + 1);
    return {true, value};
  }

  return {success, rest};
}

std::pair<bool, std::string> KVClient::ExecuteCmd(const std::string &command,
                                                  const std::string &key,
                                                  const std::string &value) {
  if (!EnsureConnect()) {
    return {false, last_error_};
  }

  std::string request = command + " " + key;
  if (!value.empty()) {
    request += " " + value;
  }

  if (!SendRequest(request)) {
    return {false, last_error_};
  }

  std::string response;
  if (!ReceiveResponse(response)) {
    return {false, last_error_};
  }

  return ParseResponse(response);
}

} // namespace tiny_kv
