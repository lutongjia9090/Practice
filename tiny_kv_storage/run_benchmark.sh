#!/bin/bash
#
# Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
# Author: Tongjia Lu (tobijah@163.com)
#

# Build benchmark program
bazelisk build //src/benchmark:kv_server_benchmark

# Create directory and copy binary files
mkdir -p ./bin/benchmark
rm -rf ./bin/benchmark/kv_server_benchmark
mv bazel-bin/src/benchmark/kv_server_benchmark ./bin/benchmark/

bazelisk build //src/grpc_server:grpc_kv_server_main
rm -rf ./bin/grpc_kv_server_main
mv bazel-bin/src/grpc_server/grpc_kv_server_main ./bin/

# Start gRPC service in background
./bin/grpc_kv_server_main > grpc_server.log 2>&1 &
SERVER_PID=$!

sleep 2
echo -e "${GREEN}[INFO] gRPC service started, PID: ${SERVER_PID}${NC}"

./bin/benchmark/kv_server_benchmark --benchmark_out=./kv_server_benchmark.json --benchmark_out_format=json --benchmark_time_unit=ms

echo -e "${GREEN}[INFO] Benchmark test completed. Results saved to kv_server_benchmark.json${NC}"

kill $SERVER_PID
wait $SERVER_PID 2>/dev/null
echo -e "${GREEN}[INFO] gRPC service stopped${NC}"
