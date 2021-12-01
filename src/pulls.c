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

#include <assert.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <sn/sn.h>
#include <ghcli/config.h>
#include <ghcli/editor.h>
#include <ghcli/curl.h>
#include <ghcli/json_util.h>
#include <ghcli/pulls.h>
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

void
ghcli_pulls_free(ghcli_pull *it, int n)
{
    for (int i = 0; i < n; ++i) {
        free((void *)it[i].title);
        free((void *)it[i].state);
        free((void *)it[i].creator);
    }
}

int
ghcli_get_prs(const char *org, const char *reponame, bool all, ghcli_pull **out)
{
    int                 count       = 0;
    json_stream         stream      = {0};
    ghcli_fetch_buffer  json_buffer = {0};
    char               *url         = NULL;

    url = sn_asprintf(
        "https://api.github.com/repos/%s/%s/pulls?per_page=100&state=%s",
        org, reponame, all ? "all" : "open");
    ghcli_fetch(url, &json_buffer);

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

    }

    free(json_buffer.data);
    free(url);
    json_close(&stream);

    return count;
}

void
ghcli_print_pr_table(FILE *stream, ghcli_pull *pulls, int pulls_size)
{
    if (pulls_size == 0) {
        fprintf(stream, "No Pull Requests\n");
        return;
    }

    fprintf(
        stream,
        "%6s  %6s  %6s  %20s  %-s\n",
        "NUMBER", "STATE", "MERGED", "CREATOR", "TITLE");

    for (int i = 0; i < pulls_size; ++i) {
        fprintf(stream, "%6d  %6s  %6s  %20s  %-s\n",
                pulls[i].number, pulls[i].state,
                sn_bool_yesno(pulls[i].merged),
                pulls[i].creator, pulls[i].title);
    }
}

void
ghcli_print_pr_diff(
    FILE *stream,
    const char *org,
    const char *reponame,
    int pr_number)
{
    char *url = NULL;
    url = sn_asprintf(
        "https://api.github.com/repos/%s/%s/pulls/%d",
        org, reponame, pr_number);
    ghcli_curl(stream, url, "Accept: application/vnd.github.v3.diff");
    free(url);
}

static void
ghcli_pull_parse_summary(json_stream *input, ghcli_pull_summary *out)
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

static void
ghcli_print_pr_summary(FILE *out, ghcli_pull_summary *it)
{
    fprintf(out,
            "   NUMBER : %d\n"
            "    TITLE : %s\n"
            "  CREATED : %s\n"
            "   AUTHOR : %s\n"
            "    STATE : %s\n"
            " COMMENTS : %d\n"
            "  ADD:DEL : %d:%d\n"
            "  COMMITS : %d\n"
            "  CHANGED : %d\n"
            "   MERGED : %s\n"
            "MERGEABLE : %s\n"
            "    DRAFT : %s\n"
            "   LABELS : ",
            it->number, it->title, it->created_at,
            it->author, it->state, it->comments,
            it->additions, it->deletions, it->commits, it->changed_files,
            sn_bool_yesno(it->merged),
            sn_bool_yesno(it->mergeable),
            sn_bool_yesno(it->draft));

    if (it->labels_size) {
        fprintf(out, SV_FMT, SV_ARGS(it->labels[0]));

        for (size_t i = 1; i < it->labels_size; ++i)
            fprintf(out, ", "SV_FMT, SV_ARGS(it->labels[i]));
    } else {
        fputs("none", out);
    }
    fputs("\n\n", out);

    if (it->body)
        pretty_print(it->body, 4, 80, out);
}

static void
parse_commit_author_field(json_stream *input, ghcli_commit *it)
{
    enum json_type key_type, value_type;
    const char *key;

    json_next(input);

    while ((key_type = json_next(input)) == JSON_STRING) {
        size_t len;
        key = json_get_string(input, &len);

        if (strncmp(key, "name", len) == 0)
            it->author = get_string(input);
        else if (strncmp(key, "email", len))
            it->email = get_string(input);
        else if (strncmp(key, "date", len))
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
parse_commit_commit_field(json_stream *input, ghcli_commit *it)
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
            parse_commit_author_field(input, it);
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

/**
 * Parse a single commit
 */
static void
ghcli_parse_commit(json_stream *input, ghcli_commit *it)
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
            parse_commit_commit_field(input, it);
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

/**
 * Fetch and extract a list of commits.
 * Returns the number of extracted commits in out.
 */
static int
ghcli_get_pull_commits(const char *url, ghcli_commit **out)
{
    int                count       = 0;
    json_stream        stream      = {0};
    ghcli_fetch_buffer json_buffer = {0};

    ghcli_fetch(url, &json_buffer);
    json_open_buffer(&stream, json_buffer.data, json_buffer.length);
    json_set_streaming(&stream, true);

    enum json_type next_token = json_next(&stream);

    while ((next_token = json_peek(&stream)) != JSON_ARRAY_END) {
        if (next_token != JSON_OBJECT)
            errx(1, "Unexpected non-object in commit list");

        *out = realloc(*out, (count + 1) * sizeof(ghcli_commit));
        ghcli_commit *it = &(*out)[count];
        ghcli_parse_commit(&stream, it);
        count += 1;
    }

    json_close(&stream);
    free(json_buffer.data);
    return count;
}

/**
 * Get a copy of the first line of the passed string.
 */
static char *
cut_newline(const char *_it)
{
    char *it = strdup(_it);
    char *foo = it;
    while (*foo) {
        if (*foo == '\n') {
            *foo = 0;
            break;
        }
        foo += 1;
    }

    return it;
}

static void
ghcli_print_commits_table(FILE *stream, ghcli_commit *commits, int commits_size)
{
    if (commits_size == 0) {
        fprintf(stream, "No commits\n");
        return;
    }

    fprintf(
        stream,
        "%8.8s  %-15.15s  %-20.20s  %16.16s  %-s\n",
        "SHA", "AUTHOR", "EMAIL", "DATE", "MESSAGE");

    for (int i = 0; i < commits_size; ++i) {
        char *message = cut_newline(commits[i].message);
        fprintf(stream, "%8.8s  %-15.15s  %-20.20s  %16.16s  %-s\n",
                commits[i].sha,
                commits[i].author,
                commits[i].date,
                commits[i].email,
                message);
        free(message);
    }
}

static void
ghcli_commits_free(ghcli_commit *it, int size)
{
    for (int i = 0; i < size; ++i) {
        free((void *)it[i].sha);
        free((void *)it[i].message);
        free((void *)it[i].date);
        free((void *)it[i].author);
        free((void *)it[i].email);
    }

    free(it);
}

void
ghcli_pulls_summary_free(ghcli_pull_summary *it)
{
    free((void *)it->author);
    free((void *)it->state);
    free((void *)it->title);
    free((void *)it->body);
    free((void *)it->created_at);
    free((void *)it->commits_link);

    for (size_t i = 0; i < it->labels_size; ++i)
        free(it->labels[i].data);
}

/**
 * Fetch and print information about a Pull request.
 */
void
ghcli_pr_summary(
    FILE *out,
    const char *org,
    const char *reponame,
    int pr_number)
{
    json_stream         stream       = {0};
    ghcli_fetch_buffer  json_buffer  = {0};
    char               *url          = NULL;
    ghcli_pull_summary  result       = {0};
    ghcli_commit       *commits      = NULL;
    int                 commits_size = 0;

    /* General info */
    url = sn_asprintf(
        "https://api.github.com/repos/%s/%s/pulls/%d",
        org, reponame, pr_number);
    ghcli_fetch(url, &json_buffer);

    json_open_buffer(&stream, json_buffer.data, json_buffer.length);
    json_set_streaming(&stream, true);

    ghcli_pull_parse_summary(&stream, &result);

    json_close(&stream);
    free(url);
    free(json_buffer.data);

    /* Commits */
    url = sn_asprintf(
        "https://api.github.com/repos/%s/%s/pulls/%d/commits",
        org, reponame, pr_number);
    commits_size = ghcli_get_pull_commits(url, &commits);

    ghcli_print_pr_summary(out, &result);
    ghcli_pulls_summary_free(&result);

    fprintf(out, "\nCOMMITS\n");
    ghcli_print_commits_table(out, commits, commits_size);

    ghcli_commits_free(commits, commits_size);
    free(url);
}

static void
pr_init_user_file(FILE *stream, void *_opts)
{
    ghcli_submit_pull_options *opts = _opts;
    fprintf(
        stream,
        "# PR TITLE : "SV_FMT"\n"
        "# Enter PR comments below.\n"
        "# All lines starting with '#' will be discarded.\n",
        SV_ARGS(opts->title)
    );
}

static sn_sv
ghcli_pr_get_user_message(ghcli_submit_pull_options *opts)
{
    return ghcli_editor_get_user_message(pr_init_user_file, opts);
}

void
ghcli_pr_submit(ghcli_submit_pull_options opts)
{
    ghcli_fetch_buffer  json_buffer  = {0};

    sn_sv body = ghcli_pr_get_user_message(&opts);
    opts.body = ghcli_json_escape(body);

    fprintf(stdout,
            "The following PR will be created:\n"
            "\n"
            "TITLE   : "SV_FMT"\n"
            "BASE    : "SV_FMT"\n"
            "HEAD    : "SV_FMT"\n"
            "IN      : "SV_FMT"\n"
            "MESSAGE :\n"SV_FMT"\n",
            SV_ARGS(opts.title),SV_ARGS(opts.to),
            SV_ARGS(opts.from), SV_ARGS(opts.in),
            SV_ARGS(body));

    fputc('\n', stdout);

    if (!opts.always_yes)
        if (!sn_yesno("Do you want to continue?"))
            errx(1, "PR aborted.");

    ghcli_perform_submit_pr(opts, &json_buffer);

    ghcli_print_html_url(json_buffer);

    free(body.data);
    free(opts.body.data);
    free(json_buffer.data);
}

void
ghcli_pr_merge(FILE *out, const char *org, const char *reponame, int pr_number)
{
    json_stream         stream      = {0};
    ghcli_fetch_buffer  json_buffer = {0};
    char               *url         = NULL;
    const char         *data        = "{}";
    enum json_type      next;
    size_t              len;
    const char         *message;
    const char         *key;

    url = sn_asprintf(
        "https://api.github.com/repos/%s/%s/pulls/%d/merge",
        org, reponame, pr_number);
    ghcli_fetch_with_method("PUT", url, data, &json_buffer);
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

            return;
        } else {
            next = json_next(&stream);
        }
    }
}

void
ghcli_pr_close(const char *org, const char *reponame, int pr_number)
{
    ghcli_fetch_buffer  json_buffer = {0};
    const char         *url         = NULL;
    const char         *data        = NULL;

    url  = sn_asprintf(
        "https://api.github.com/repos/%s/%s/pulls/%d",
        org, reponame, pr_number);
    data = sn_asprintf("{ \"state\": \"closed\"}");

    ghcli_fetch_with_method("PATCH", url, data, &json_buffer);

    free(json_buffer.data);
    free((void *)url);
    free((void *)data);
}

void
ghcli_pr_reopen(const char *org, const char *reponame, int pr_number)
{
    ghcli_fetch_buffer  json_buffer = {0};
    const char         *url         = NULL;
    const char         *data        = NULL;

    url  = sn_asprintf(
        "https://api.github.com/repos/%s/%s/pulls/%d",
        org, reponame, pr_number);
    data = sn_asprintf("{ \"state\": \"open\"}");

    ghcli_fetch_with_method("PATCH", url, data, &json_buffer);

    free(json_buffer.data);
    free((void *)url);
    free((void *)data);
}
