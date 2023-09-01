#!/bin/sh -x

die() {
    printf -- "error: $@\n" >&2
    exit 1
}

which gcli > /dev/null 2>&1 || die "gcli is not in PATH"

gcli -t github api /repos/herrhotzenplotz/gcli/issues/115 > samples/github_simple_issue.json
gcli -t github api /repos/herrhotzenplotz/gcli/labels/bug > samples/github_simple_label.json
gcli -t github api /repos/herrhotzenplotz/gcli/pulls/113 > samples/github_simple_pull.json
