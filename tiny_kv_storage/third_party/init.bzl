# Copyright 2025 Tobijah.lu Inc. All Rights Reserved.
# Author: Tongjia Lu (tobijah@163.com)
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

def _init_http_repos():
    if "com_github_google_benchmark" not in native.existing_rules():
        http_archive(
            name = "com_github_google_benchmark",
            urls = [
                "https://github.com/google/benchmark/archive/refs/tags/v1.7.1.tar.gz",
            ],
            strip_prefix = "benchmark-1.7.1",
            sha256 = "6430e4092653380d9dc4ccb45a1e2dc9259d581f4866dc0759713126056bc1d7",
            build_file = "//third_party:com_github_google_benchmark.BUILD",
        )

def init():
    _init_http_repos()
    _init_local_repos()
