# Copyright 2024 Tobijah.lu Inc. All Rights Reserved.
# Author: Tongjia Lu (lutonjia@163.com)
#

# buildifier: disable=module-docstring
load(
    "@bazel_tools//tools/build_defs/repo:http.bzl",
    "http_archive",
)

def _init_local_repos():
    local_repos = [
        ("com_google_googletest", "third_party/googletest-1.11.0"),
        ("com_google_absl", "third_party/abseil-cpp-20210324.2"),
    ]
    for name, path in local_repos:
        if name not in native.existing_rules():
            native.local_repository(name = name, path = path)

def init():
    _init_local_repos()
