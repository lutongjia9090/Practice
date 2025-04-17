# Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
# Author: Tongjia Lu (tobijah@163.com)
#

licenses(["notice"])

cc_library(
    name = "gflags",
    srcs = [
        "src/config.h",
        "src/defines.h",
        "src/gflags.cc",
        "src/gflags_completions.cc",
        "src/gflags_reporting.cc",
        "src/mutex.h",
        "src/util.h",
    ],
    hdrs = [
        "include/gflags/gflags.h",
        "include/gflags/gflags_completions.h",
        "include/gflags/gflags_declare.h",
        "include/gflags/gflags_gflags.h",
    ],
    copts = [
        "-std=c++11",
        "-w",
        "-pthread",
    ],
    includes = [
        "include",
    ],
    linkopts = [
        "-pthread",
    ],
    linkstatic = True,
    visibility = [
        "//visibility:public",
    ],
)
