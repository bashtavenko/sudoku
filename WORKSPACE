load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

all_content = """\
filegroup(
    name = "all",
    srcs = glob(["**"]),
    visibility = ["//visibility:public"],
)
"""

http_archive(
    name = "opencv",
    build_file_content = all_content,
    sha256 = "cbf47ecc336d2bff36b0dcd7d6c179a9bb59e805136af6b9670ca944aef889bd",
    strip_prefix = "opencv-4.8.0",
    urls = ["https://github.com/opencv/opencv/archive/refs/tags/4.8.0.tar.gz"],
)
