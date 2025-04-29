// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (tobijah@163.com)
//

#include "kv_client.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sstream>
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
  std::string request_with_delimiter = request + "\r\n";
  ssize_t bytes_sent = send(socket_fd_, request_with_delimiter.c_str(),
                            request_with_delimiter.length(), 0);
  if (bytes_sent != static_cast<ssize_t>(request_with_delimiter.length())) {
    last_error_ = "Failed to send request.";
    return false;
  }
  return true;
}

bool KVClient::ReceiveResponse(std::string &response) {
  char buffer[1024] = {0};
  std::string full_response;

  while (true) {
    ssize_t bytes_read = recv(socket_fd_, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
      last_error_ = "Failed to receive response.";
      return false;
    }

    full_response.append(buffer, bytes_read);

    size_t pos = full_response.find("\r\n");
    if (pos != std::string::npos) {
      response = full_response.substr(0, pos);
      return true;
    }
  }
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

std::unordered_map<std::string, std::string>
KVClient::MultiGet(const std::vector<std::string> &keys) {
  auto[success, response] = ExecuteMultiCmd("MGET", keys);
  std::unordered_map<std::string, std::string> result;

  if (success) {
    std::istringstream iss(response);
    std::string key, value;
    while (iss >> key >> value) {
      result[key] = value;
    }
  }

  return result;
}

bool KVClient::MultiPut(
    const std::unordered_map<std::string, std::string> &kv_pairs) {
  std::vector<std::string> keys;
  for (const auto & [ key, _ ] : kv_pairs) {
    keys.push_back(key);
  }
  auto[success, _] = ExecuteMultiCmd("MPUT", keys, kv_pairs);
  return success;
}

bool KVClient::MultiDelete(const std::vector<std::string> &keys) {
  auto[success, _] = ExecuteMultiCmd("MDEL", keys);
  return success;
}

std::pair<bool, std::string> KVClient::ExecuteMultiCmd(
    const std::string &command, const std::vector<std::string> &keys,
    const std::unordered_map<std::string, std::string> &values) {
  if (!EnsureConnect()) {
    return {false, last_error_};
  }

  std::string request = command;

  if (command == "MPUT") {
    for (const auto &key : keys) {
      request += " " + key + " " + values.at(key);
    }
  } else {
    for (const auto &key : keys) {
      request += " " + key;
    }
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
