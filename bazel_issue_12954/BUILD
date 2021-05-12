
load(":def.bzl", "show_srcs")


filegroup(
    name = "files",
    srcs = glob(
        ["dir1/**"],
        exclude_directories = 0,
    ),
)


show_srcs(
    name = "glob_in_filegroup",
    srcs = [":files"],
)

show_srcs(
    name = "glob_in_srcs",
    srcs = glob(
       ["dir1/**"],
    ),
)

show_srcs(
    name = "glob_with_dirs",
    srcs = glob(
       ["dir1/**"],
       exclude_directories = 0,
    ),
)

show_srcs(
    name = "explicit_files",
    srcs = [
        "dir1",
        "dir1/file1.txt",
        "dir1/dir2",
        "dir1/dir2/file2.txt",
    ],
)
