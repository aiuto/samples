Repro for https://github.com/bazelbuild/bazel/issues/12954

File.is_directory seems always broken. That seems to big to fail, but....


```
bazel build :*
more bazel-bin/*.out | cat
```

I would expect dir1 and dir1 to be directories, but they are not
