# Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
# Author: Tongjia Lu (tobijah@163.com)
#

load(
    "//:build_config.bzl",
    "custom_cc_library",
)

load("@com_github_grpc_grpc//bazel:cc_grpc_library.bzl", "cc_grpc_library")

proto_library(
    name = "kv_service_proto",
    srcs = ["kv_service.proto"],
)

cc_proto_library(
    name = "kv_service_cc_proto",
    deps = [":kv_service_proto"],
)

cc_grpc_library(
    name = "kv_service_grpc",
    srcs = [":kv_service_proto"],
    grpc_only = True,
    deps = [":kv_service_cc_proto"],
)

custom_cc_library(
    name = "kv_proto",
    deps = [
        ":kv_service_cc_proto",
        ":kv_service_grpc",
    ],
    copts = [
        "-std=c++17",
        "-w",  # 禁用警告，避免gRPC头文件的问题
    ],
    visibility = ["//visibility:public"],
)
