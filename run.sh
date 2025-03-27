#! /bin/bash
#
# Copyright 2024 Tobijah.lu Inc. All Rights Reserved.
# Author: Tongjia Lu (lutonjia@163.com)
#

set -e
cd $(dirname $0)

# Build && Run
source /opt/rh/devtoolset-9/enable
bazelisk build //project/test:test
mv bazel-bin/project/test/test ./bin/test
./bin/test

bazelisk build //project/hashmap_benchmark:benchmark
mv bazel-bin/project/hashmap_benchmark/benchmark ./bin/benchmark
./bin/benchmark

bazelisk test //project/thread_pool:thread_pool_test

bazelisk build //project/net_server:server
bazelisk build //project/net_server:client
rm -rf ./bin/server ./bin/client
mv bazel-bin/project/net_server/server ./bin/server
mv bazel-bin/project/net_server/client ./bin/client
