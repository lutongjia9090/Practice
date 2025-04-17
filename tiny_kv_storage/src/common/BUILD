# Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
# Author: Tongjia Lu (tobijah@163.com)
#

load(
    "//:build_config.bzl",
    "custom_cc_library",
    "custom_cc_test",
)

custom_cc_library(
    name = "kv_common",
    hdrs = [
        "kv_common.h",
    ],
)

custom_cc_library(
    name = "storage_engine",
    srcs = [
        "storage_engine.cc",
    ],
    hdrs = [
        "storage_engine.h",
    ],
)

custom_cc_test(
    name = "storage_engine_test",
    srcs = ["storage_engine_test.cc"],
    deps = [
        "storage_engine",
        "@com_google_googletest//:gtest_main",
    ],
)
