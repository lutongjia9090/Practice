// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (tobijah@163.com)
//

#include "grpc_kv_client.h"
#include <gflags/gflags.h>
#include <sstream>
#include <stdio.h>
#include <string>
#include <unordered_map>
#include <vector>

DEFINE_string(server, "127.0.0.1:8080",
              "The server address in the format of host:port");
DEFINE_bool(async, false, "Use async client API");

namespace tiny_kv {

const char *kUsageMessage = R"(
Options:
  --async                     Use asynchronous client API

Supported Client Commands:
  get <key>                   Get the value of a key
  put <key> <value>           Set a key-value pair
  delete <key>                Delete a key-value pair
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
  explicit CommandProcessor(GrpcKVClient *client) : client_(client) {
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
    command_handlers_["delete"] = &CommandProcessor::HandleDeleteCommand;
    command_handlers_["mget"] = &CommandProcessor::HandleMultiGetCommand;
    command_handlers_["mput"] = &CommandProcessor::HandleMultiPutCommand;
    command_handlers_["mdel"] = &CommandProcessor::HandleMultiDeleteCommand;
  }

  void HandleCallback(bool success, const std::string &value) {
    if (success) {
      printf("(value) %s\n", value.c_str());
    } else {
      printf("(error) %s\n", client_->GetLastError().c_str());
    }
  }

  void HandleCallback(bool success) {
    if (success) {
      printf("(async) OK\n");
    } else {
      printf("(async error) %s\n", client_->GetLastError().c_str());
    }
  }

  void
  HandleCallback(bool success,
                 const std::unordered_map<std::string, std::string> &result) {
    if (success && !result.empty()) {
      for (const auto & [ key, value ] : result) {
        printf("%s: %s\n", key.c_str(), value.c_str());
      }
    } else {
      printf("error or empty result) %s\n", client_->GetLastError().c_str());
    }
  }

  void HandleGetCommand(std::istringstream &iss) {
    std::string key;
    if (!(iss >> key)) {
      PrintUsage("get");
      return;
    }

    if (FLAGS_async) {
      client_->AsyncGet(key, [this](bool success, const std::string &value) {
        HandleCallback(success, value);
      });
    } else {
      auto[success, value] = client_->Get(key);
      HandleCallback(success, value);
    }
  }

  void HandlePutCommand(std::istringstream &iss) {
    std::string key, value;
    if (!(iss >> key) || !(iss >> value)) {
      PrintUsage("put");
      return;
    }

    if (FLAGS_async) {
      client_->AsyncPut(key, value,
                        [this](bool success) { HandleCallback(success); });
    } else {
      HandleCallback(client_->Put(key, value));
    }
  }

  void HandleDeleteCommand(std::istringstream &iss) {
    std::string key;
    if (!(iss >> key)) {
      PrintUsage("delete");
      return;
    }

    if (FLAGS_async) {
      client_->AsyncDelete(key,
                           [this](bool success) { HandleCallback(success); });
    } else {
      HandleCallback(client_->Delete(key));
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

    if (FLAGS_async) {
      client_->AsyncMultiGet(
          keys,
          [this](bool success,
                 const std::unordered_map<std::string, std::string> &result) {
            HandleCallback(success, result);
          });
    } else {
      auto result = client_->MultiGet(keys);
      HandleCallback(!result.empty(), result);
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

    if (FLAGS_async) {
      client_->AsyncMultiPut(kv_pairs,
                             [this](bool success) { HandleCallback(success); });
    } else {
      HandleCallback(client_->MultiPut(kv_pairs));
    }
  }

  void HandleMultiDeleteCommand(std::istringstream &iss) {
    std::vector<std::string> keys;
    std::string key;

    while (iss >> key) {
      keys.push_back(key);
    }

    if (keys.empty()) {
      PrintUsage("mdel");
      return;
    }

    if (FLAGS_async) {
      client_->AsyncMultiDelete(
          keys, [this](bool success) { HandleCallback(success); });
    } else {
      HandleCallback(client_->MultiDelete(keys));
    }
  }

  void PrintUsage(const std::string &cmd) {
    static const std::unordered_map<std::string, std::string> usage_map = {
        {"get", "Usage: get <key>"},
        {"put", "Usage: put <key> <value>"},
        {"delete", "Usage: delete <key>"},
        {"mget", "Usage: mget <key1> <key2> ..."},
        {"mput", "Usage: mput <key1> <value1> <key2> <value2> ..."},
        {"mdel", "Usage: mdel <key1> <key2> ..."}};

    auto it = usage_map.find(cmd);
    if (it != usage_map.end()) {
      printf("%s\n", it->second.c_str());
    }
  }

private:
  GrpcKVClient *client_;
  std::unordered_map<std::string, CommandHandler> command_handlers_;
};

} // namespace tiny_kv

int main(int argc, char *argv[]) {
  gflags::SetUsageMessage(tiny_kv::kUsageMessage);
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  tiny_kv::GrpcKVClient client(FLAGS_server);

  if (!client.Connect()) {
    printf("Error: Failed to connect to server: %s\n",
           client.GetLastError().c_str());
    return 1;
  }

  printf("Connected to %s\n", FLAGS_server.c_str());
  printf("Type 'exit' to quit\n");

  tiny_kv::CommandProcessor processor(&client);
  processor.RunInteractive();

  client.Shutdown();

  google::ShutDownCommandLineFlags();

  return 0;
}
