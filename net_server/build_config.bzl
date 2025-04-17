# Copyright 2024 Tobijah.lu Inc. All Rights Reserved.
# Author: Tongjia Lu (lutonjia@163.com)
#

# buildifier: disable=module-docstring
LOCAL_DEFINES = [
]

COPTS = [
    "-Wall",
    "-Wextra",
    "-Werror",
    "-pedantic",
    "-pthread",
]

LINKOPTS = [
    "-pthread",
]

def _get_copts(copts, std):
    if std:
        copts = [copt for copt in copts if not copt.startswith("-std=")]
        copts.append("-std=" + std)
    else:
        std_copts = [copt for copt in copts if copt.startswith("-std=")]
        if len(std_copts) == 0:
            copts.append("-std=c++17")
    return copts

def custom_cc_library(
        name,
        srcs = [],
        hdrs = [],
        copts = [],
        includes = [],
        linkopts = [],
        local_defines = [],
        visibility = [
            "//visibility:public",
        ],
        deps = [],
        alwayslink = False,
        std = "c++17",
        **kwargs):
    copts = _get_copts(COPTS + copts, std)
    native.cc_library(
        name = name,
        srcs = srcs,
        hdrs = hdrs,
        copts = copts,
        includes = includes,
        linkopts = LINKOPTS + linkopts,
        linkstatic = True,
        local_defines = LOCAL_DEFINES + local_defines,
        visibility = visibility,
        deps = deps,
        alwayslink = alwayslink,
        **kwargs
    )

def custom_cc_binary(
        name,
        srcs = [],
        copts = [],
        linkopts = [],
        local_defines = [],
        visibility = [
            "//visibility:public",
        ],
        deps = [],
        std = "c++17",
        **kwargs):
    copts = _get_copts(COPTS + copts, std)
    native.cc_binary(
        name = name,
        srcs = srcs,
        copts = copts,
        linkopts = LINKOPTS + linkopts,
        linkstatic = True,
        local_defines = LOCAL_DEFINES + local_defines,
        visibility = visibility,
        deps = deps,
        **kwargs
    )

def custom_cc_test(
        name,
        srcs,
        copts = [],
        linkopts = [],
        local_defines = [],
        visibility = [
            "//visibility:public",
        ],
        deps = [],
        std = "c++17",
        **kwargs):
    if "@com_google_googletest//:gtest_main" not in deps:
        deps = [
            "@com_google_googletest//:gtest_main",
        ] + deps
    copts = _get_copts(COPTS + copts, std)
    native.cc_test(
        name = name,
        srcs = srcs,
        copts = copts,
        linkopts = LINKOPTS + linkopts,
        linkstatic = True,
        local_defines = LOCAL_DEFINES + local_defines,
        visibility = visibility,
        deps = deps,
        **kwargs
    )

def custom_cc_benchmark(
        name,
        srcs,
        copts = [],
        linkopts = [],
        local_defines = [],
        visibility = [
            "//visibility:public",
        ],
        deps = [],
        std = "c++17",
        **kwargs):
    if "@com_github_google_benchmark//:benchmark_main" not in deps:
        deps = [
            "@com_github_google_benchmark//:benchmark_main",
        ] + deps
    copts = _get_copts(COPTS + copts, std)
    native.cc_binary(
        name = name,
        srcs = srcs,
        copts = copts,
        linkopts = LINKOPTS + linkopts,
        linkstatic = True,
        local_defines = LOCAL_DEFINES + local_defines,
        visibility = visibility,
        deps = deps,
        **kwargs
    )
