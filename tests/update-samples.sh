#!/bin/sh -x

die() {
    printf -- "error: $@\n" >&2
    exit 1
}

which gcli > /dev/null 2>&1 || die "gcli is not in PATH"
which jq > /dev/null 2>&1 || die "jq is not in PATH"

gcli -t github api /repos/herrhotzenplotz/gcli/issues/115 > samples/github_simple_issue.json
gcli -t github api /repos/herrhotzenplotz/gcli/labels/bug > samples/github_simple_label.json
gcli -t github api /repos/herrhotzenplotz/gcli/pulls/113 > samples/github_simple_pull.json
gcli -t github api /repos/herrhotzenplotz/gcli/milestones/1 > samples/github_simple_milestone.json
gcli -t github api /repos/herrhotzenplotz/gcli/releases/116031718 > samples/github_simple_release.json
gcli -t github api /repos/herrhotzenplotz/gcli > samples/github_simple_repo.json
gcli -t github api /repos/quick-lint/quick-lint-js/forks | jq '.[] | select(.id == 639263592)' > samples/github_simple_fork.json
gcli -t github api /repos/herrhotzenplotz/gcli/issues/comments/1424392601 > samples/github_simple_comment.json

gcli -t gitlab api /projects/herrhotzenplotz%2Fgcli/merge_requests/216 > samples/gitlab_simple_merge_request.json
gcli -t gitlab api /projects/herrhotzenplotz%2Fgcli/issues/193 > samples/gitlab_simple_issue.json
gcli -t gitlab api /projects/herrhotzenplotz%2Fgcli/labels/24376073 > samples/gitlab_simple_label.json
