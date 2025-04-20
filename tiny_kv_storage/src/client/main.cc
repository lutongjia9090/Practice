// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (tobijah@163.com)
//

#include "kv_client.h"
#include "src/common/kv_common.h"
#include <cstdio>
#include <gflags/gflags.h>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace tiny_kv {

DEFINE_string(server_ip, "127.0.0.1", "The server ip address");
DEFINE_int32(server_port, 8080, "The server port");

const char *kUsageMessage = R"(
Supported Client Commands:
  get <key>                   Get the value of a key
  put <key> <value>           Set a key-value pair
  del <key>                   Delete a key-value pair
  mget <key1> <key2> ...      Get multiple keys
  mput <key1> <value1> <key2> <value2> ...  Set multiple key-value pairs
  mdel <key1> <key2> ...      Delete multiple keys
  exit                        Exit the client
)";

/************************************************************************/
/* CommandProcessor */
/************************************************************************/
class CommandProcessor {
public:
  explicit CommandProcessor(KVClient *client) : client_(client) {
    InitCommandHandlers();
  }

  bool ProcessCommand(const std::string &cmdline) {
    if (cmdline.empty()) {
      return true;
    }

    std::istringstream iss(cmdline);
    std::string command;
    iss >> command;

    if (command == "exit") {
      return false;
    }

    auto it = command_handlers_.find(command);
    if (it != command_handlers_.end()) {
      (this->*(it->second))(iss);
    } else {
      printf("Invalid command\n");
    }

    return true;
  }

  void RunInteractive() {
    std::string line;
    while (true) {
      printf("> ");
      std::getline(std::cin, line);

      if (!ProcessCommand(line)) {
        break;
      }
    }
  }

private:
  using CommandHandler = void (CommandProcessor::*)(std::istringstream &);

  void InitCommandHandlers() {
    command_handlers_["get"] = &CommandProcessor::HandleGetCommand;
    command_handlers_["put"] = &CommandProcessor::HandlePutCommand;
    command_handlers_["del"] = &CommandProcessor::HandleDelCommand;
    command_handlers_["mget"] = &CommandProcessor::HandleMultiGetCommand;
    command_handlers_["mput"] = &CommandProcessor::HandleMultiPutCommand;
    command_handlers_["mdel"] = &CommandProcessor::HandleMultiDelCommand;
  }

  void HandleGetCommand(std::istringstream &iss) {
    std::string key;
    if (!(iss >> key)) {
      PrintUsage("get");
      return;
    }

    auto[success, value] = client_->Get(key);
    if (success) {
      printf("(value) %s\n", value.c_str());
    } else {
      printf("(error) %s\n", value.c_str());
    }
  }

  void HandlePutCommand(std::istringstream &iss) {
    std::string key;
    std::string value;
    if (!(iss >> key) || !(iss >> value)) {
      PrintUsage("put");
      return;
    }

    if (!client_->Put(key, value)) {
      printf("(error) %s\n", client_->GetLastError().c_str());
    } else {
      printf("OK\n");
    }
  }

  void HandleDelCommand(std::istringstream &iss) {
    std::string key;
    if (!(iss >> key)) {
      PrintUsage("del");
      return;
    }

    if (!client_->Delete(key)) {
      printf("(error) %s\n", client_->GetLastError().c_str());
    } else {
      printf("OK\n");
    }
  }

  void HandleMultiGetCommand(std::istringstream &iss) {
    std::vector<std::string> keys;
    std::string key;

    while (iss >> key) {
      keys.push_back(key);
    }

    if (keys.empty()) {
      PrintUsage("mget");
      return;
    }

    auto result = client_->MultiGet(keys);
    if (result.empty()) {
      printf("(empty result or error)\n");
    } else {
      for (const auto &[key, value] : result) {
        printf("%s: %s\n", key.c_str(), value.c_str());
      }
    }
  }

  void HandleMultiPutCommand(std::istringstream &iss) {
    std::unordered_map<std::string, std::string> kv_pairs;
    std::string key, value;

    while (iss >> key >> value) {
      kv_pairs[key] = value;
    }

    if (kv_pairs.empty()) {
      PrintUsage("mput");
      return;
    }

    if (!client_->MultiPut(kv_pairs)) {
      printf("(error) %s\n", client_->GetLastError().c_str());
    } else {
      printf("OK\n");
    }
  }

  void HandleMultiDelCommand(std::istringstream &iss) {
    std::vector<std::string> keys;
    std::string key;

    while (iss >> key) {
      keys.push_back(key);
    }

    if (keys.empty()) {
      PrintUsage("mdel");
      return;
    }

    if (!client_->MultiDelete(keys)) {
      printf("(error) %s\n", client_->GetLastError().c_str());
    } else {
      printf("OK\n");
    }
  }

  void PrintUsage(const std::string &cmd) {
    static const std::unordered_map<std::string, std::string> usage_map = {
        {"get", "Usage: get <key>"},
        {"put", "Usage: put <key> <value>"},
        {"del", "Usage: del <key>"},
        {"mget", "Usage: mget <key1> <key2> ..."},
        {"mput", "Usage: mput <key1> <value1> <key2> <value2> ..."},
        {"mdel", "Usage: mdel <key1> <key2> ..."}};

    auto it = usage_map.find(cmd);
    if (it != usage_map.end()) {
      printf("%s\n", it->second.c_str());
    }
  }

private:
  KVClient *client_;
  std::unordered_map<std::string, CommandHandler> command_handlers_;
};

int main(int argc, char **argv) {
  gflags::SetUsageMessage(kUsageMessage);

  gflags::ParseCommandLineFlags(&argc, &argv, true);

  gflags::ShowUsageWithFlagsRestrict(argv[0], "src/client/main");

  auto client =
      std::make_unique<tiny_kv::KVClient>(FLAGS_server_ip, FLAGS_server_port);
  if (!client->Connect()) {
    printf("Error: %s", client->GetLastError().c_str());
    return 1;
  }

  CommandProcessor processor(client.get());
  processor.RunInteractive();

  gflags::ShutDownCommandLineFlags();
  return 0;
}

} // namespace tiny_kv

int main(int argc, char **argv) { return tiny_kv::main(argc, argv); }
