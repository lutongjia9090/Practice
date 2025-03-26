# Copyright 2024 Tobijah.lu Inc. All Rights Reserved.
# Author: Tongjia Lu (lutonjia@163.com)
#

load(
    "//:build_config.bzl",
    "custom_cc_library",
    "custom_cc_test",
)

custom_cc_library(
    name = "thread_pool",
    hdrs = [
        "thread_pool.h",
    ],
    srcs = [
        "thread_pool.cc",
    ],
    deps = [
        "@concurrentqueue",
    ],
)

custom_cc_test(
    name = "thread_pool_test",
    srcs = [
        "thread_pool_test.cc",
    ],
    deps = [
        ":thread_pool",
    ],
)
