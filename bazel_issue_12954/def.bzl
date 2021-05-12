# Are my inputs files or directories?

def _show_srcs_impl(ctx):
    content = []
    # Do this both ways, but it should not matter.
    for src in ctx.attr.srcs:
        for f in src.files.to_list():
            content.append('from files: %s: is_dir=%s\n' % (f.path, f.is_directory))
    for src in ctx.attr.srcs:
        for f in src[DefaultInfo].files.to_list():
            content.append('from DefaultInfo: %s: is_dir=%s\n' % (f.path, f.is_directory))

    ctx.actions.write(ctx.outputs.out, ''.join(content))
    return [
        DefaultInfo(
            files = depset([ctx.outputs.out]),
        ),
    ]


_show_srcs = rule(
    implementation = _show_srcs_impl,
    attrs = {
        "srcs": attr.label_list(allow_files = True),
        "out": attr.output(mandatory = True),
    },
)


def show_srcs(name, **kwargs):
    if not 'out' in kwargs:
        kwargs['out'] = name + ".out"
    _show_srcs(name=name, **kwargs)
