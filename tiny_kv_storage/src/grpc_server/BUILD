# Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
# Author: Tongjia Lu (tobijah@163.com)
#

load(
    "//:build_config.bzl",
    "custom_cc_binary",
    "custom_cc_library",
)

custom_cc_library(
    name = "async_grpc_kv_server_lib",
    srcs = [
        "async_grpc_kv_server.cc",
    ],
    hdrs = [
        "async_grpc_kv_server.h",
    ],
    deps = [
        "//src/common:storage_engine",
        "//src/proto:kv_proto",
        "@com_github_grpc_grpc//:grpc++",
    ],
)

custom_cc_binary(
    name = "grpc_kv_server_main",
    srcs = ["main.cc"],
    deps = [
        ":async_grpc_kv_server_lib",
        "@com_github_gflags_gflags//:gflags",
    ],
)
