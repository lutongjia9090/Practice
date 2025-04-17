# Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
# Author: Tongjia Lu (tobijah@163.com)
#

licenses(["notice"])

LOCAL_DEFINES = [
    "HAVE_POSIX_REGEX=1",
    "HAVE_STD_REGEX=1",
    "HAVE_STEADY_CLOCK=1",
]

COPTS = [
    "-std=c++11",
    "-w",
    "-pthread",
    "-fstrict-aliasing",
]

LINKOPTS = [
    "-pthread",
]

cc_library(
    name = "benchmark",
    srcs = glob(
        [
            "src/**/*.cc",
            "src/**/*.h",
        ],
        exclude = [
            "src/benchmark_main.cc",
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
    local_defines = LOCAL_DEFINES,
    visibility = [
        "//visibility:public",
    ],
)

cc_library(
    name = "benchmark_main",
    srcs = [
        "src/benchmark_main.cc",
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
    local_defines = LOCAL_DEFINES,
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":benchmark",
    ],
)
