# Copyright 2024 Tobijah.lu Inc. All Rights Reserved.
# Author: Tongjia Lu (tobijah@163.com)
#

licenses(["notice"])

COPTS = [
    "-std=c++11",
    "-w",
    "-pthread",
]

LINKOPTS = [
    "-pthread",
]

cc_library(
    name = "gtest",
    srcs = glob(
        [
            "src/*.cc",
            "src/**/*.h",
        ],
        exclude = [
            "src/gmock_main.cc",
        ],
    ),
    hdrs = glob([
        "include/**/*.h",
    ]),
    copts = COPTS,
    includes = [
        "include",
    ],
    linkopts = LINKOPTS,
    linkstatic = True,
    visibility = [
        "//visibility:public",
    ],
)

cc_library(
    name = "gtest_main",
    srcs = [
        "src/gmock_main.cc",
    ],
    hdrs = glob([
        "include/**/*.h",
    ]),
    copts = COPTS,
    includes = [
        "include",
    ],
    linkopts = LINKOPTS,
    linkstatic = True,
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":gtest",
    ],
)
