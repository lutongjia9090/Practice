# Copyright 2024 Tobijah.lu Inc. All Rights Reserved.
# Author: Tongjia Lu (lutonjia@163.com)
#

load(
    "//:build_config.bzl",
    "custom_cc_binary",
)

custom_cc_binary(
    name = "server",
    srcs = [
        "tcp_epoll.cc",
    ],
    copts = [
        "-std=c++17",
    ],
)

custom_cc_binary(
    name = "client",
    srcs = [
        "client.cc",
    ],
    copts = [
        "-std=c++17",
    ],
)
