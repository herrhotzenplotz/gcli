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

#include <ghcli/curl.h>
#include <ghcli/gitlab/config.h>
#include <ghcli/gitlab/repos.h>
#include <ghcli/gitlab/merge_requests.h>
#include <ghcli/json_util.h>

#include <pdjson/pdjson.h>

static void
gitlab_parse_mr_entry(json_stream *input, ghcli_pull *it)
{
    if (json_next(input) != JSON_OBJECT)
        errx(1, "Expected Issue Object");

    enum json_type key_type;
    while ((key_type = json_next(input)) == JSON_STRING) {
        size_t          len        = 0;
        const char     *key        = json_get_string(input, &len);
        enum json_type  value_type = 0;

        if (strncmp("title", key, len) == 0)
            it->title = get_string(input);
        else if (strncmp("state", key, len) == 0)
            it->state = get_string(input);
        else if (strncmp("iid", key, len) == 0)
            it->number = get_int(input);
        else if (strncmp("id", key, len) == 0)
            it->id = get_int(input);
        else if (strncmp("merged_at", key, len) == 0)
            it->merged = json_next(input) == JSON_STRING;
        else if (strncmp("author", key, len) == 0)
            it->creator = get_user(input);
        else {
            value_type = json_next(input);

            switch (value_type) {
            case JSON_ARRAY:
                json_skip_until(input, JSON_ARRAY_END);
                break;
            case JSON_OBJECT:
                json_skip_until(input, JSON_OBJECT_END);
                break;
            default:
                break;
            }
        }
    }
}

int
gitlab_get_mrs(
    const char  *owner,
    const char  *reponame,
    bool         all,
    int          max,
    ghcli_pull **out)
{
    int                 count       = 0;
    json_stream         stream      = {0};
    ghcli_fetch_buffer  json_buffer = {0};
    char               *url         = NULL;
    char               *next_url    = NULL;

    url = sn_asprintf(
        "%s/projects/%s%%2F%s/merge_requests%s",
        gitlab_get_apibase(),
        owner, reponame,
        all ? "" : "?state=opened");

    do {
        ghcli_fetch(url, &next_url, &json_buffer);

        json_open_buffer(&stream, json_buffer.data, json_buffer.length);
        json_set_streaming(&stream, true);

        enum json_type next_token = json_next(&stream);

        while ((next_token = json_peek(&stream)) != JSON_ARRAY_END) {

            switch (next_token) {
            case JSON_ERROR:
                errx(1, "Parser error: %s", json_get_error(&stream));
                break;
            case JSON_OBJECT: {
                *out = realloc(*out, sizeof(ghcli_pull) * (count + 1));
                ghcli_pull *it = &(*out)[count];
                memset(it, 0, sizeof(ghcli_pull));
                gitlab_parse_mr_entry(&stream, it);
                count += 1;
            } break;
            default:
                errx(1, "Unexpected json type in response");
                break;
            }

            if (count == max)
                break;
        }

        free(json_buffer.data);
        free(url);
        json_close(&stream);
    } while ((url = next_url) && (max == -1 || count < max));

    free(url);

    return count;
}

void
gitlab_print_pr_diff(
    FILE       *stream,
    const char *owner,
    const char *reponame,
    int         pr_number)
{
    (void)owner;
    (void)reponame;
    (void)pr_number;

    fprintf(stream,
            "note : Getting the diff of a Merge Request is not "
            "supported on GitLab. Blame the Gitlab people.\n");
}

void
gitlab_mr_merge(
    FILE       *stream,
    const char *owner,
    const char *reponame,
    int         mr_number)
{
    ghcli_fetch_buffer  buffer = {0};
    char               *url    = NULL;
    const char         *data   = "{}";

    /* PUT /projects/:id/merge_requests/:merge_request_iid/merge */
    url = sn_asprintf(
        "%s/projects/%s%%2F%s/merge_requests/%d/merge",
        gitlab_get_apibase(),
        owner, reponame, mr_number);

    ghcli_fetch_with_method("PUT", url, data, NULL, &buffer);

    fprintf(stream, "Merged.\n");
    ghcli_print_html_url(buffer);

    free(buffer.data);
    free(url);
}

static void
gitlab_pull_parse_summary(json_stream *input, ghcli_pull_summary *out)
{
    enum json_type key_type, value_type;
    const char *key;

    json_next(input);

    while ((key_type = json_next(input)) == JSON_STRING) {
        size_t len;
        key = json_get_string(input, &len);

        if (strncmp("title", key, len) == 0)
            out->title = get_string(input);
        else if (strncmp("state", key, len) == 0)
            out->state = get_string(input);
        else if (strncmp("description", key, len) == 0)
            out->body = get_string(input);
        else if (strncmp("created_at", key, len) == 0)
            out->created_at = get_string(input);
        else if (strncmp("iid", key, len) == 0)
            out->number = get_int(input);
        else if (strncmp("id", key, len) == 0)
            out->id = get_int(input);
        else if (strncmp("labels", key, len) == 0)
            out->labels_size = ghcli_read_label_list(input, &out->labels);
        else if (strncmp("user_notes_count", key, len) == 0)
            out->comments = get_int(input);
#if 0
        else if (strncmp("changes_count", key, len) == 0)
            out->commits = get_int(input);
        else if (strncmp("additions", key, len) == 0)
            out->additions = get_int(input);
        else if (strncmp("deletions", key, len) == 0)
            out->deletions = get_int(input);
        else if (strncmp("changed_files", key, len) == 0)
            out->changed_files = get_int(input);
        else if (strncmp("merged", key, len) == 0)
            out->merged = get_bool(input);
#endif
        else if (strncmp("merge_status", key, len) == 0)
            /* TODO: hack */
            out->mergeable = sn_sv_eq_to(get_sv(input), "can_be_merged");
        else if (strncmp("draft", key, len) == 0)
            out->draft = get_bool(input);
        else if (strncmp("author", key, len) == 0)
            out->author = get_user(input);
        else {
            value_type = json_next(input);

            switch (value_type) {
            case JSON_ARRAY:
                json_skip_until(input, JSON_ARRAY_END);
                break;
            case JSON_OBJECT:
                json_skip_until(input, JSON_OBJECT_END);
                break;
            default:
                break;
            }
        }
    }

    if (key_type != JSON_OBJECT_END)
        errx(1, "Unexpected object key type");
}

void
gitlab_get_pull_summary(
    const char         *owner,
    const char         *repo,
    int                 pr_number,
    ghcli_pull_summary *out)
{
    json_stream         stream       = {0};
    ghcli_fetch_buffer  json_buffer  = {0};
    char               *url          = NULL;

    /* GET /projects/:id/merge_requests/:merge_request_iid */
    url = sn_asprintf(
        "%s/projects/%s%%2F%s/merge_requests/%d",
        gitlab_get_apibase(),
        owner, repo, pr_number);
    ghcli_fetch(url, NULL, &json_buffer);

    json_open_buffer(&stream, json_buffer.data, json_buffer.length);
    json_set_streaming(&stream, true);

    gitlab_pull_parse_summary(&stream, out);

    json_close(&stream);
    free(url);
    free(json_buffer.data);
}

static void
gitlab_parse_commit(json_stream *input, ghcli_commit *it)
{
    enum json_type key_type, value_type;
    const char *key;

    json_next(input);

    while ((key_type = json_next(input)) == JSON_STRING) {
        size_t len;
        key = json_get_string(input, &len);

        if (strncmp(key, "short_id", len) == 0)
            it->sha = get_string(input);
        else if (strncmp(key, "title", len) == 0)
            it->message = get_string(input);
        else if (strncmp(key, "created_at", len) == 0)
            it->date = get_string(input);
        else if (strncmp(key, "author_name", len) == 0)
            it->author = get_string(input);
        else if (strncmp(key, "author_email", len) == 0)
            it->email = get_string(input);
        else {
            value_type = json_next(input);

            switch (value_type) {
            case JSON_ARRAY:
                json_skip_until(input, JSON_ARRAY_END);
                break;
            case JSON_OBJECT:
                json_skip_until(input, JSON_OBJECT_END);
                break;
            default:
                break;
            }
        }
    }

    if (key_type != JSON_OBJECT_END)
        errx(1, "Unexpected non-string object key");
}

int
gitlab_get_pull_commits(
    const char    *owner,
    const char    *repo,
    int            pr_number,
    ghcli_commit **out)
{
    char              *url         = NULL;
    int                count       = 0;
    json_stream        stream      = {0};
    ghcli_fetch_buffer json_buffer = {0};

    /* GET /projects/:id/merge_requests/:merge_request_iid/commits */
    url = sn_asprintf(
        "%s/projects/%s%%2F%s/merge_requests/%d/commits",
        gitlab_get_apibase(),
        owner, repo, pr_number);

    ghcli_fetch(url, NULL, &json_buffer);
    json_open_buffer(&stream, json_buffer.data, json_buffer.length);
    json_set_streaming(&stream, true);

    enum json_type next_token = json_next(&stream);

    while ((next_token = json_peek(&stream)) != JSON_ARRAY_END) {
        if (next_token != JSON_OBJECT)
            errx(1, "Unexpected non-object in commit list");

        *out = realloc(*out, (count + 1) * sizeof(ghcli_commit));
        ghcli_commit *it = &(*out)[count];
        gitlab_parse_commit(&stream, it);
        count += 1;
    }

    json_close(&stream);
    free(url);
    free(json_buffer.data);
    return count;
}

void
gitlab_mr_close(const char *owner, const char *reponame, int pr_number)
{
    ghcli_fetch_buffer  json_buffer = {0};
    const char         *url         = NULL;
    const char         *data        = NULL;

    url  = sn_asprintf(
        "%s/projects/%s%%2F%s/merge_requests/%d",
        gitlab_get_apibase(),
        owner, reponame, pr_number);
    data = sn_asprintf("{ \"state_event\": \"close\"}");

    ghcli_fetch_with_method("PUT", url, data, NULL, &json_buffer);

    free(json_buffer.data);
    free((void *)url);
    free((void *)data);
}

void
gitlab_mr_reopen(const char *owner, const char *reponame, int pr_number)
{
    ghcli_fetch_buffer  json_buffer = {0};
    const char         *url         = NULL;
    const char         *data        = NULL;

    url  = sn_asprintf(
        "%s/projects/%s%%2F%s/merge_requests/%d",
        gitlab_get_apibase(),
        owner, reponame, pr_number);
    data = sn_asprintf("{ \"state_event\": \"reopen\"}");

    ghcli_fetch_with_method("PUT", url, data, NULL, &json_buffer);

    free(json_buffer.data);
    free((void *)url);
    free((void *)data);
}

void
gitlab_perform_submit_mr(
    ghcli_submit_pull_options  opts,
    ghcli_fetch_buffer        *out)
{
    /* Note: this doesn't really allow merging into repos with
     * different names. We need to figure out a way to make this
     * better for both github and gitlab. */
    ghcli_repo target        = {0};
    sn_sv      target_owner  = {0};
    sn_sv      target_branch = {0};
    sn_sv      source_owner  = {0};
    sn_sv      repo          = {0};
    sn_sv      source_branch = {0};

    repo                  = opts.in;
    target_owner          = sn_sv_chop_until(&repo, '/');
    repo.data            += 1;
    repo.length          -= 1;
    target_branch         = opts.to;
    source_branch         = opts.from;
    source_owner          = sn_sv_chop_until(&source_branch, ':');
    source_branch.length -= 1;
    source_branch.data   += 1;

    /* Figure out the project id */
    gitlab_get_repo(target_owner, repo, &target);

    /* TODO : JSON Injection */
    char *post_fields = sn_asprintf(
        "{\"source_branch\":\""SV_FMT"\",\"target_branch\":\""SV_FMT"\", "
        "\"title\": \""SV_FMT"\", \"description\": \""SV_FMT"\", "
        "\"target_project_id\": %d }",
        SV_ARGS(source_branch),
        SV_ARGS(target_branch),
        SV_ARGS(opts.title),
        SV_ARGS(opts.body),
        target.id);
    char *url         = sn_asprintf(
        "%s/projects/"SV_FMT"%%2F"SV_FMT"/merge_requests",
        gitlab_get_apibase(),
        SV_ARGS(source_owner), SV_ARGS(repo));

    ghcli_fetch_with_method("POST", url, post_fields, NULL, out);
    free(post_fields);
    free(url);
}
