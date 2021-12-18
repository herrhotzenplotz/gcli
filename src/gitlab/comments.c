/*
 * Copyright 2021 Nico Sonack <nsonack@outlook.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <ghcli/gitlab/comments.h>
#include <ghcli/gitlab/config.h>

void
gitlab_perform_submit_comment(
    ghcli_submit_comment_opts  opts,
    ghcli_fetch_buffer        *out)
{
    const char *type  = NULL;

    switch (opts.target_type) {
    case ISSUE_COMMENT:
        type = "issues";
        break;
    case PR_COMMENT:
        type = "merge_requests";
        break;
    }

    char *post_fields = sn_asprintf(
        "{ \"body\": \""SV_FMT"\" }",
        SV_ARGS(opts.message));
    char *url         = sn_asprintf(
        "%s/projects/%s%%2F%s/%s/%d/notes",
        gitlab_get_apibase(),
        opts.owner, opts.repo, type, opts.target_id);

    ghcli_fetch_with_method("POST", url, post_fields, NULL, out);
    free(post_fields);
    free(url);
}
