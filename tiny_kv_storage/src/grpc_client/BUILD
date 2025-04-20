# Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
# Author: Tongjia Lu (tobijah@163.com)
#

load(
    "//:build_config.bzl",
    "custom_cc_binary",
    "custom_cc_library",
)

custom_cc_library(
    name = "grpc_kv_client_lib",
    srcs = [
        "grpc_kv_client.cc",
    ],
    hdrs = [
        "grpc_kv_client.h",
    ],
    deps = [
        "//src/proto:kv_proto",
        "@com_github_grpc_grpc//:grpc++",
    ],
)

custom_cc_binary(
    name = "grpc_kv_client_main",
    srcs = ["main.cc"],
    deps = [
        ":grpc_kv_client_lib",
        "@com_github_gflags_gflags//:gflags",
    ],
)
