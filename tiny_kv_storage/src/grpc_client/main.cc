// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (tobijah@163.com)
//

#include "grpc_kv_client.h"
#include <gflags/gflags.h>
#include <sstream>
#include <stdio.h>
#include <string>
#include <unordered_map>

DEFINE_string(server, "127.0.0.1:8080",
              "The server address in the format of host:port");

namespace tiny_kv {

const char *kUsageMessage = R"(
Supported Client Commands:
  get <key>          Get the value of a key
  put <key> <value>  Set a key-value pair
  delete <key>       Delete a key-value pair
  exit               Exit the client
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
      printf("(error) %s\n", client_->GetLastError().c_str());
    }
  }

  void HandlePutCommand(std::istringstream &iss) {
    std::string key, value;
    if (!(iss >> key) || !(iss >> value)) {
      PrintUsage("put");
      return;
    }

    if (client_->Put(key, value)) {
      printf("OK\n");
    } else {
      printf("(error) %s\n", client_->GetLastError().c_str());
    }
  }

  void HandleDeleteCommand(std::istringstream &iss) {
    std::string key;
    if (!(iss >> key)) {
      PrintUsage("delete");
      return;
    }

    if (client_->Delete(key)) {
      printf("OK\n");
    } else {
      printf("(error) %s\n", client_->GetLastError().c_str());
    }
  }

  void PrintUsage(const std::string &cmd) {
    static const std::unordered_map<std::string, std::string> usage_map = {
        {"get", "Usage: get <key>"},
        {"put", "Usage: put <key> <value>"},
        {"delete", "Usage: delete <key>"}};

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
