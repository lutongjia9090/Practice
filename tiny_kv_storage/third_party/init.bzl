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

    if "com_github_gflags_gflags" not in native.existing_rules():
        http_archive(
            name = "com_github_gflags_gflags",
            urls = [
                "https://github.com/gflags/gflags/archive/refs/tags/v2.2.2.tar.gz",
            ],
            strip_prefix = "gflags-2.2.2",
            sha256 = "34af2f15cf7367513b352bdcd2493ab14ce43692d2dcd9dfc499492966c64dcf",
        )

    if "com_github_grpc_grpc" not in native.existing_rules():
        http_archive(
            name = "com_github_grpc_grpc",
            urls = [
                "https://github.com/grpc/grpc/archive/refs/tags/v1.45.2.tar.gz",
            ],
            strip_prefix = "grpc-1.45.2",
            sha256 = "e18b16f7976aab9a36c14c38180f042bb0fd196b75c9fd6a20a2b5f934876ad6",
            patches = [
                "//third_party:com_github_grpc_grpc.patch",
            ],
            patch_args = [
                "-p1",
            ],
        )

    if "com_google_absl" not in native.existing_rules():
        http_archive(
            name = "com_google_absl",
            urls = [
                "https://github.com/abseil/abseil-cpp/archive/refs/tags/20210324.2.tar.gz",
            ],
            strip_prefix = "abseil-cpp-20210324.2",
            sha256 = "59b862f50e710277f8ede96f083a5bb8d7c9595376146838b9580be90374ee1f",
            patches = [
                "//third_party:com_google_absl.patch",
            ],
            patch_args = [
                "-p1",
            ],
        )

    if "com_google_protobuf" not in native.existing_rules():
        http_archive(
            name = "com_google_protobuf",
            urls = [
                "https://github.com/protocolbuffers/protobuf/archive/refs/tags/v3.17.3.tar.gz",
            ],
            strip_prefix = "protobuf-3.17.3",
            sha256 = "c6003e1d2e7fefa78a3039f19f383b4f3a61e81be8c19356f85b6461998ad3db",
            patches = [
                "//third_party:com_google_protobuf.patch",
            ],
            patch_args = [
                "-p1",
            ],
        )

def init():
    _init_http_repos()
    _init_local_repos()
