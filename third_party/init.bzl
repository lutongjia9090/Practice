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

def _init_http_repos():
    if "parallel_hashmap" not in native.existing_rules():
        http_archive(
            name = "parallel_hashmap",  # no consensus
            urls = [
                "https://github.com/greg7mdp/parallel-hashmap/archive/refs/tags/v1.3.8.tar.gz",
            ],
            strip_prefix = "parallel-hashmap-1.3.8",
            sha256 = "c4562ea360dc1dcaddd96a0494c753400364a52c7aa9750de49d8e6a222d28d3",
            build_file = "//third_party:parallel_hashmap.BUILD",
            patch_cmds = [
                "mkdir -p include/parallel_hashmap",
                "mv parallel_hashmap/*.h include/parallel_hashmap",
            ],
        )

    if "unordered_dense" not in native.existing_rules():
        http_archive(
            name = "unordered_dense",  # no consensus
            urls = [
                "https://github.com/martinus/unordered_dense/archive/refs/tags/v4.5.0.tar.gz",
            ],
            strip_prefix = "unordered_dense-4.5.0",
            sha256 = "2364ce4bc4c23bd02549bbb3a7572d881684cd46057f3737fd53be53669743aa",
            build_file = "//third_party:unordered_dense.BUILD",
        )

    if "concurrentqueue" not in native.existing_rules():
        http_archive(
            name = "concurrentqueue",  # no consensus
            urls = [
                "https://github.com/cameron314/concurrentqueue/archive/refs/tags/v1.0.3.tar.gz",
            ],
            strip_prefix = "concurrentqueue-1.0.3",
            sha256 = "eb37336bf9ae59aca7b954db3350d9b30d1cab24b96c7676f36040aa76e915e8",
            build_file = "//third_party:concurrentqueue.BUILD",
            patch_cmds = [
                "mkdir include",
                "mv *.h internal include",
            ],
        )

def init():
    _init_http_repos()
    _init_local_repos()
