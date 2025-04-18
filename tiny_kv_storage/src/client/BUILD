# Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
# Author: Tongjia Lu (tobijah@163.com)
#

load(
    "//:build_config.bzl",
    "custom_cc_library",
    "custom_cc_binary",
)

custom_cc_library(
    name = "kv_client_lib",
    srcs = [
        "kv_client.cc",
    ],
    hdrs = [
        "kv_client.h",
    ],
)

custom_cc_binary(
    name = "kv_client_main",
    srcs = [
        "main.cc",
    ],
    includes = [
        "external/com_github_gflags_gflags/include",
    ],
    deps = [
        ":kv_client_lib",
        "//src/common:kv_common",
        "@com_github_gflags_gflags//:gflags",
    ],
)
