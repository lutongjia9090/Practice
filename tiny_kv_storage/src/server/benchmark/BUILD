# Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
# Author: Tongjia Lu (tobijah@163.com)
#

load(
    "//:build_config.bzl",
    "custom_cc_binary",
)

custom_cc_binary(
    name = "kv_benchmark",
    srcs = [
        "kv_server_benchmark.cc",
    ],
    deps = [
        "//src/client:kv_client_lib",
        "@com_github_gflags_gflags//:gflags",
    ],
)
