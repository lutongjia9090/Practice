#! /bin/bash
#
# Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
# Author: Tongjia Lu (tobijah@163.com)
#

set -e
cd $(dirname $0)

# Build && Run
source /opt/rh/devtoolset-9/enable

bazelisk build //src/server:kv_server_main //src/client:kv_client_main //src/grpc_server:grpc_kv_server_main //src/grpc_client:grpc_kv_client_main

mkdir -p ./bin
rm -rf ./bin/kv_server_main ./bin/kv_client_main ./bin/grpc_kv_server_main ./bin/grpc_kv_client_main
mv bazel-bin/src/server/kv_server_main ./bin/
mv bazel-bin/src/client/kv_client_main ./bin/
mv bazel-bin/src/grpc_server/grpc_kv_server_main ./bin/
mv bazel-bin/src/grpc_client/grpc_kv_client_main ./bin/
