// Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
// Author: Tongjia Lu (tobijah@163.com)
//

#include "async_grpc_kv_server.h"
#include <csignal>
#include <gflags/gflags.h>
#include <stdio.h>
#include <string>

DEFINE_string(ip, "127.0.0.1", "The server ip");
DEFINE_int32(port, 8080, "The server port");
DEFINE_string(storage_type, "memory", "Storage type: 'memory' or 'file'");
DEFINE_string(storage_path, "test.db",
              "Path to database file when using file storage");

static tiny_kv::AsyncGrpcKVServer *g_server = nullptr;

void HandleSignal(int signal) {
  printf("Received signal %d, shutting down...\n", signal);
  if (g_server) {
    g_server->Stop();
  }
}

int main(int argc, char *argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  std::string server_address = FLAGS_ip + ":" + std::to_string(FLAGS_port);

  tiny_kv::AsyncGrpcKVServer server(server_address, FLAGS_storage_type,
                                    FLAGS_storage_path);
  g_server = &server;

  std::signal(SIGINT, HandleSignal);
  std::signal(SIGTERM, HandleSignal);

  server.Start();

  server.Wait();

  google::ShutDownCommandLineFlags();

  return 0;
}
