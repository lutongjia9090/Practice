// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (tobijah@163.com)
//

#pragma once

#include <string>
#include <utility>
#include <vector>
#include <unordered_map>

namespace tiny_kv {

/************************************************************************/
/* KVClient */
/************************************************************************/
class KVClient {
public:
  KVClient(const std::string &server_ip, int server_port);
  ~KVClient();

  bool Connect();
  std::pair<bool, std::string> Get(const std::string &key);
  bool Put(const std::string &key, const std::string &value);
  bool Delete(const std::string &key);

  std::unordered_map<std::string, std::string> MultiGet(const std::vector<std::string> &keys);
  bool MultiPut(const std::unordered_map<std::string, std::string> &kv_pairs);
  bool MultiDelete(const std::vector<std::string> &keys);

  std::string GetLastError() const;

private:
  void Disconnect();
  bool EnsureConnect();
  bool SendRequest(const std::string &request);
  bool ReceiveResponse(std::string &response);
  std::pair<bool, std::string> ParseResponse(const std::string &response);
  std::pair<bool, std::string> ExecuteCmd(const std::string &command,
                                          const std::string &key,
                                          const std::string &value = "");
  std::pair<bool, std::string> ExecuteMultiCmd(const std::string &command,
                                               const std::vector<std::string> &keys,
                                               const std::unordered_map<std::string, std::string> &values = {});

private:
  std::string server_ip_;
  int server_port_;
  bool connected_;
  int socket_fd_;
  std::string last_error_;
};

} // namespace tiny_kv
