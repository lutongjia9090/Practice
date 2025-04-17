# Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
# Author: Tongjia Lu (tobijah@163.com)
#

# buildifier: disable=module-docstring
load(
    "//:build_config_third_party.bzl",
    "third_party_cc_binary",
    "third_party_cc_library",
)

def absl_cc_binary(
        name,
        srcs = [],
        visibility = [
            "//visibility:public",
        ],
        deps = [],
        **kwargs):
    third_party_cc_binary(
        name = name,
        srcs = srcs,
        visibility = visibility,
        deps = deps,
        warning = "enable_all",
        **kwargs
    )

def absl_cc_library(
        name,
        srcs = [],
        hdrs = [],
        includes = [],
        visibility = [
            "//visibility:public",
        ],
        deps = [],
        **kwargs):
    third_party_cc_library(
        name = name,
        srcs = srcs,
        hdrs = hdrs,
        includes = includes,
        visibility = visibility,
        deps = deps,
        warning = "enable_all",
        **kwargs
    )
