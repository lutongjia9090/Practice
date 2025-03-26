# Copyright 2024 Tobijah.lu Inc. All Rights Reserved.
# Author: Tongjia Lu (lutonjia@163.com)
#

load(
    "//:build_config.bzl",
    "custom_cc_binary",
)

custom_cc_binary(
    name = "benchmark",
    srcs = [
        "hashmap_benchmark.cc",
    ],
    copts = [
        "-std=c++17",
    ],
    deps = [
        "@com_google_absl//absl/container:flat_hash_map",
        "@com_google_absl//absl/container:node_hash_map",
        "@parallel_hashmap",
        "@unordered_dense",
    ],
)
