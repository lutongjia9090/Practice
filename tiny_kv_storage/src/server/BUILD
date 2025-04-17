# Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
# Author: Tongjia Lu (tobijah@163.com)
#

load(
    "//:build_config.bzl",
    "custom_cc_library",
    "custom_cc_binary",
)

custom_cc_library(
    name = "kv_server_lib",
    srcs = [
        "kv_server.cc",
    ],
    hdrs = [
        "kv_server.h",
    ],
    deps = [
        "//src/common:kv_common",
        "//src/common:storage_engine",
    ],
)

custom_cc_binary(
    name = "kv_server_main",
    srcs = [
        "main.cc",
    ],
    includes = [
        "external/com_github_gflags_gflags/include",
    ],
    deps = [
        ":kv_server_lib",
        "@com_github_gflags_gflags//:gflags",
    ],
)
