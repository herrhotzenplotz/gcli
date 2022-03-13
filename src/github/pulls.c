/*
 * Copyright 2021 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <ghcli/config.h>
#include <ghcli/curl.h>
#include <ghcli/gitconfig.h>
#include <ghcli/github/config.h>
#include <ghcli/github/pulls.h>
#include <ghcli/json_util.h>

#include <pdjson/pdjson.h>

static void
parse_pull_entry(json_stream *input, ghcli_pull *it)
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
        else if (strncmp("number", key, len) == 0)
            it->number = get_int(input);
        else if (strncmp("id", key, len) == 0)
            it->id = get_int(input);
        else if (strncmp("merged_at", key, len) == 0)
            it->merged = json_next(input) == JSON_STRING;
        else if (strncmp("user", key, len) == 0)
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
github_get_prs(
    const char  *owner,
    const char  *repo,
    bool         all,
    int          max,
    ghcli_pull **out)
{
    int                 count       = 0;
    json_stream         stream      = {0};
    ghcli_fetch_buffer  json_buffer = {0};
    char               *url         = NULL;
    char               *next_url    = NULL;
    char               *e_owner     = NULL;
    char               *e_repo      = NULL;

    e_owner = ghcli_urlencode(owner);
    e_repo  = ghcli_urlencode(repo);

    url = sn_asprintf(
        "%s/repos/%s/%s/pulls?state=%s",
        github_get_apibase(),
        e_owner, e_repo, all ? "all" : "open");

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
                parse_pull_entry(&stream, it);
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
    free(e_owner);
    free(e_repo);

    return count;
}

void
github_print_pr_diff(
    FILE       *stream,
    const char *owner,
    const char *repo,
    int         pr_number)
{
    char *url     = NULL;
    char *e_owner = NULL;
    char *e_repo  = NULL;

    e_owner = ghcli_urlencode(owner);
    e_repo  = ghcli_urlencode(repo);

    url = sn_asprintf(
        "%s/repos/%s/%s/pulls/%d",
        github_get_apibase(),
        e_owner, e_repo, pr_number);
    ghcli_curl(stream, url, "Accept: application/vnd.github.v3.diff");

    free(e_owner);
    free(e_repo);
    free(url);
}

void
github_pr_merge(
    FILE       *out,
    const char *owner,
    const char *repo,
    int         pr_number)
{
    json_stream         stream      = {0};
    ghcli_fetch_buffer  json_buffer = {0};
    char               *url         = NULL;
    char               *e_owner     = NULL;
    char               *e_repo      = NULL;
    const char         *data        = "{}";
    enum json_type      next;
    size_t              len;
    const char         *message;
    const char         *key;

    e_owner = ghcli_urlencode(owner);
    e_repo  = ghcli_urlencode(repo);

    url = sn_asprintf(
        "%s/repos/%s/%s/pulls/%d/merge",
        github_get_apibase(),
        e_owner, e_repo, pr_number);
    ghcli_fetch_with_method("PUT", url, data, NULL, &json_buffer);
    json_open_buffer(&stream, json_buffer.data, json_buffer.length);
    json_set_streaming(&stream, true);

    next = json_next(&stream);

    while ((next = json_next(&stream)) == JSON_STRING) {
        key = json_get_string(&stream, &len);

        if (strncmp(key, "message", len) == 0) {

            next = json_next(&stream);
            message  = json_get_string(&stream, &len);

            fprintf(out, "%.*s\n", (int)len, message);

            json_close(&stream);
            free(json_buffer.data);
            free(url);
            free(e_owner);
            free(e_repo);

            return;
        } else {
            next = json_next(&stream);
        }
    }
}

void
github_pr_close(const char *owner, const char *repo, int pr_number)
{
    ghcli_fetch_buffer  json_buffer = {0};
    char               *url         = NULL;
    char               *e_owner     = NULL;
    char               *e_repo      = NULL;
    char               *data        = NULL;

    e_owner = ghcli_urlencode(owner);
    e_repo  = ghcli_urlencode(repo);

    url  = sn_asprintf(
        "%s/repos/%s/%s/pulls/%d",
        github_get_apibase(),
        e_owner, e_repo, pr_number);
    data = sn_asprintf("{ \"state\": \"closed\"}");

    ghcli_fetch_with_method("PATCH", url, data, NULL, &json_buffer);

    free(json_buffer.data);
    free(url);
    free(e_repo);
    free(e_owner);
    free(data);
}

void
github_pr_reopen(const char *owner, const char *repo, int pr_number)
{
    ghcli_fetch_buffer  json_buffer = {0};
    char               *url         = NULL;
    char               *data        = NULL;
    char               *e_owner     = NULL;
    char               *e_repo      = NULL;

    e_owner = ghcli_urlencode(owner);
    e_repo  = ghcli_urlencode(repo);

    url  = sn_asprintf(
        "%s/repos/%s/%s/pulls/%d",
        github_get_apibase(),
        e_owner, e_repo, pr_number);
    data = sn_asprintf("{ \"state\": \"open\"}");

    ghcli_fetch_with_method("PATCH", url, data, NULL, &json_buffer);

    free(json_buffer.data);
    free(url);
    free(data);
    free(e_owner);
    free(e_repo);
}

void
github_perform_submit_pr(
    ghcli_submit_pull_options  opts,
    ghcli_fetch_buffer        *out)
{
    sn_sv repo = ghcli_urlencode_sv(opts.in);
    /* TODO : JSON Injection */
    char *post_fields = sn_asprintf(
        "{\"head\":\""SV_FMT"\",\"base\":\""SV_FMT"\", "
        "\"title\": \""SV_FMT"\", \"body\": \""SV_FMT"\" }",
        SV_ARGS(opts.from),
        SV_ARGS(opts.to),
        SV_ARGS(opts.title),
        SV_ARGS(opts.body));
    char *url         = sn_asprintf(
        "%s/repos/"SV_FMT"/pulls",
        github_get_apibase(),
        SV_ARGS(repo));

    ghcli_fetch_with_method("POST", url, post_fields, NULL, out);
    free(post_fields);
    free(url);
    free(repo.data);
}

static void
github_parse_commit_author_field(json_stream *input, ghcli_commit *it)
{
    enum json_type key_type, value_type;
    const char *key;

    json_next(input);

    while ((key_type = json_next(input)) == JSON_STRING) {
        size_t len;
        key = json_get_string(input, &len);

        if (strncmp(key, "name", len) == 0)
            it->author = get_string(input);
        else if (strncmp(key, "email", len) == 0)
            it->email = get_string(input);
        else if (strncmp(key, "date", len) == 0)
            it->date = get_string(input);
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

static void
github_parse_commit_commit_field(json_stream *input, ghcli_commit *it)
{
    enum json_type key_type, value_type;
    const char *key;

    json_next(input);

    while ((key_type = json_next(input)) == JSON_STRING) {
        size_t len;
        key = json_get_string(input, &len);

        if (strncmp(key, "message", len) == 0)
            it->message = get_string(input);
        else if (strncmp(key, "author", len) == 0)
            github_parse_commit_author_field(input, it);
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

static void
github_parse_commit(json_stream *input, ghcli_commit *it)
{
    enum json_type key_type, value_type;
    const char *key;

    json_next(input);

    while ((key_type = json_next(input)) == JSON_STRING) {
        size_t len;
        key = json_get_string(input, &len);

        if (strncmp(key, "sha", len) == 0)
            it->sha = get_string(input);
        else if (strncmp(key, "commit", len) == 0)
            github_parse_commit_commit_field(input, it);
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
github_get_pull_commits(
    const char    *owner,
    const char    *repo,
    int            pr_number,
    ghcli_commit **out)
{
    char              *url         = NULL;
    char              *next_url    = NULL;
    char              *e_owner     = NULL;
    char              *e_repo      = NULL;
    int                count       = 0;
    json_stream        stream      = {0};
    ghcli_fetch_buffer json_buffer = {0};

    e_owner = ghcli_urlencode(owner);
    e_repo  = ghcli_urlencode(repo);

    url = sn_asprintf(
        "%s/repos/%s/%s/pulls/%d/commits",
        github_get_apibase(),
        e_owner, e_repo, pr_number);

    do {
        ghcli_fetch(url, &next_url, &json_buffer);
        json_open_buffer(&stream, json_buffer.data, json_buffer.length);
        json_set_streaming(&stream, true);

        enum json_type next_token = json_next(&stream);

        while ((next_token = json_peek(&stream)) != JSON_ARRAY_END) {
            if (next_token != JSON_OBJECT)
                errx(1, "Unexpected non-object in commit list");

            *out = realloc(*out, (count + 1) * sizeof(ghcli_commit));
            ghcli_commit *it = &(*out)[count];
            github_parse_commit(&stream, it);
            count += 1;
        }

        json_close(&stream);
        free(json_buffer.data);
        free(url);
    } while ((url = next_url));

    free(e_owner);
    free(e_repo);

    return count;
}

static void
github_pull_parse_summary(json_stream *input, ghcli_pull_summary *out)
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
        else if (strncmp("body", key, len) == 0)
            out->body = get_string(input);
        else if (strncmp("created_at", key, len) == 0)
            out->created_at = get_string(input);
        else if (strncmp("number", key, len) == 0)
            out->number = get_int(input);
        else if (strncmp("id", key, len) == 0)
            out->id = get_int(input);
        else if (strncmp("commits", key, len) == 0)
            out->commits = get_int(input);
        else if (strncmp("labels", key, len) == 0)
            out->labels_size = ghcli_read_label_list(input, &out->labels);
        else if (strncmp("comments", key, len) == 0)
            out->comments = get_int(input);
        else if (strncmp("additions", key, len) == 0)
            out->additions = get_int(input);
        else if (strncmp("deletions", key, len) == 0)
            out->deletions = get_int(input);
        else if (strncmp("changed_files", key, len) == 0)
            out->changed_files = get_int(input);
        else if (strncmp("merged", key, len) == 0)
            out->merged = get_bool(input);
        else if (strncmp("mergeable", key, len) == 0)
            out->mergeable = get_bool(input);
        else if (strncmp("draft", key, len) == 0)
            out->draft = get_bool(input);
        else if (strncmp("user", key, len) == 0)
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
github_get_pull_summary(
    const char         *owner,
    const char         *repo,
    int                 pr_number,
    ghcli_pull_summary *out)
{
    json_stream         stream      = {0};
    ghcli_fetch_buffer  json_buffer = {0};
    char               *url         = NULL;
    char               *e_owner     = NULL;
    char               *e_repo      = NULL;

    e_owner = ghcli_urlencode(owner);
    e_repo  = ghcli_urlencode(repo);

    url = sn_asprintf(
        "%s/repos/%s/%s/pulls/%d",
        github_get_apibase(),
        e_owner, e_repo, pr_number);
    ghcli_fetch(url, NULL, &json_buffer);

    json_open_buffer(&stream, json_buffer.data, json_buffer.length);
    json_set_streaming(&stream, true);

    github_pull_parse_summary(&stream, out);
    json_close(&stream);
    free(url);
    free(e_owner);
    free(e_repo);
    free(json_buffer.data);
}
