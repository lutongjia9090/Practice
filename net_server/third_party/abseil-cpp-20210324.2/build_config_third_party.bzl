# Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
# Author: Tongjia Lu (lutonjia@163.com)
#

# buildifier: disable=module-docstring
LOCAL_DEFINES = [
]

C_COPTS = [
    "-std=c11",
    "-pthread",
]

CC_COPTS = [
    "-std=c++11",
    "-pthread",
]

LINKOPTS = [
    "-pthread",
]

def _get_warning_copts(warning):
    if warning == "disable_all":
        return [
            "-w",
        ]
    elif warning == "enable_all":
        return [
            "-Wall",
            "-Wextra",
            "-Werror",
            "-pedantic",
        ]
    else:
        return []

def third_party_c_binary(
        name,
        srcs = [],
        copts = [],
        linkopts = [],
        linkstatic = True,
        local_defines = [],
        visibility = [
            "//visibility:public",
        ],
        deps = [],
        warning = "disable_all",
        **kwargs):
    native.cc_binary(
        name = name,
        srcs = srcs,
        copts = C_COPTS + copts + _get_warning_copts(warning),
        linkopts = LINKOPTS + linkopts,
        linkstatic = linkstatic,
        local_defines = LOCAL_DEFINES + local_defines,
        visibility = visibility,
        deps = deps,
        **kwargs
    )

def third_party_cc_binary(
        name,
        srcs = [],
        copts = [],
        linkopts = [],
        linkstatic = True,
        local_defines = [],
        visibility = [
            "//visibility:public",
        ],
        deps = [],
        warning = "disable_all",
        **kwargs):
    native.cc_binary(
        name = name,
        srcs = srcs,
        copts = CC_COPTS + copts + _get_warning_copts(warning),
        linkopts = LINKOPTS + linkopts,
        linkstatic = linkstatic,
        local_defines = LOCAL_DEFINES + local_defines,
        visibility = visibility,
        deps = deps,
        **kwargs
    )

def third_party_c_library(
        name,
        srcs = [],
        hdrs = [],
        copts = [],
        includes = [],
        linkopts = [],
        linkstatic = True,
        local_defines = [],
        visibility = [
            "//visibility:public",
        ],
        deps = [],
        alwayslink = False,
        warning = "disable_all",
        **kwargs):
    native.cc_library(
        name = name,
        srcs = srcs,
        hdrs = hdrs,
        copts = C_COPTS + copts + _get_warning_copts(warning),
        includes = includes,
        linkopts = LINKOPTS + linkopts,
        linkstatic = linkstatic,
        local_defines = LOCAL_DEFINES + local_defines,
        visibility = visibility,
        deps = deps,
        alwayslink = alwayslink,
        **kwargs
    )

def third_party_cc_library(
        name,
        srcs = [],
        hdrs = [],
        copts = [],
        includes = [],
        linkopts = [],
        linkstatic = True,
        local_defines = [],
        visibility = [
            "//visibility:public",
        ],
        deps = [],
        alwayslink = False,
        warning = "disable_all",
        **kwargs):
    native.cc_library(
        name = name,
        srcs = srcs,
        hdrs = hdrs,
        copts = CC_COPTS + copts + _get_warning_copts(warning),
        includes = includes,
        linkopts = LINKOPTS + linkopts,
        linkstatic = linkstatic,
        local_defines = LOCAL_DEFINES + local_defines,
        visibility = visibility,
        deps = deps,
        alwayslink = alwayslink,
        **kwargs
    )
