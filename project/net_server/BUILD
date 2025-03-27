# Copyright 2024 Tobijah.lu Inc. All Rights Reserved.
# Author: Tongjia Lu (lutonjia@163.com)
#

load(
    "//:build_config.bzl",
    "custom_cc_library",
    "custom_cc_binary",
)

custom_cc_library(
    name = "inet_adress",
    hdrs = [
        "inet_adress.h",
    ],
    srcs = [
        "inet_adress.cc",
    ],
)

custom_cc_library(
    name = "socket",
    hdrs = [
        "socket.h",
    ],
    srcs = [
        "socket.cc",
    ],
    deps = [
        "inet_adress",
    ],
)

custom_cc_library(
    name = "channel",
    hdrs = [
        "channel.h",
        "epoll.h",
    ],
    srcs = [
        "channel.cc",
        "epoll.cc",
    ],
    deps = [
        "inet_adress",
        "socket",
    ],
)

custom_cc_binary(
    name = "server",
    srcs = [
        "tcp_epoll.cc",
    ],
    copts = [
        "-std=c++17",
    ],
    deps = [
        "inet_adress",
        "socket",
        "channel",
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
