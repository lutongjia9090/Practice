# Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
# Author: Tongjia Lu (tobijah@163.com)
#

load(
    "//:build_config.bzl",
    "custom_cc_benchmark",
)

custom_cc_benchmark(
    name = "kv_server_benchmark",
    srcs = [
        "kv_server_benchmark.cc",
    ],
    deps = [
        "//src/common:storage_engine",
        "//src/grpc_client:grpc_kv_client_lib",
        "@com_github_google_benchmark//:benchmark",
    ],
)
