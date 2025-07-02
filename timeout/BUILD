config_setting(
    name = "windows",
    values = {"cpu": "x64_windows"},
)

config_setting(
    name = "macos",
    values = {"cpu": "darwin"},
)

config_setting(
    name = "linux",
    values = {"cpu": "k8"},
)

cc_binary(
    name = "timeout",
    srcs = ["timeout.cpp"],
    visibility = ["//visibility:public"],
    deps = [],
    copts = [
        "-Wall",
        "-Wextra",
        "-std=c++11",
    ],
) 