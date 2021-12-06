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

#include <string.h>

#include <sn/sn.h>
#include <ghcli/curl.h>
#include <ghcli/editor.h>
#include <ghcli/json_util.h>
#include <ghcli/issues.h>
#include <pdjson/pdjson.h>

static void
perform_submit_issue(
    ghcli_submit_issue_options opts,
    ghcli_fetch_buffer *out)
{
    char *post_fields = sn_asprintf(
        "{ \"title\": \""SV_FMT"\", \"body\": \""SV_FMT"\" }",
        SV_ARGS(opts.title), SV_ARGS(opts.body));
    char *url         = sn_asprintf(
        "https://api.github.com/repos/"SV_FMT"/issues",
        SV_ARGS(opts.in));

    ghcli_fetch_with_method("POST", url, post_fields, out);
    free(post_fields);
    free(url);
}

static void
parse_issue_entry(json_stream *input, ghcli_issue *it)
{
    if (json_next(input) != JSON_OBJECT)
        errx(1, "Expected Issue Object");

    while (json_next(input) == JSON_STRING) {
        size_t          len        = 0;
        const char     *key        = json_get_string(input, &len);
        enum json_type  value_type = 0;

        if (strncmp("title", key, len) == 0) {
            it->title = get_string(input);
        } else if (strncmp("state", key, len) == 0) {
            it->state = get_string(input);
        } else if (strncmp("number", key, len) == 0) {
            it->number = get_int(input);
        } else if (strncmp("id", key, len) == 0) {
            it->id = get_int(input);
        } else {
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
ghcli_issues_free(ghcli_issue *it, int size)
{
    for (int i = 0; i < size; ++i) {
        free((void *)it[i].title);
        free((void *)it[i].state);
    }

    free(it);
}

int
ghcli_get_issues(
    const char *owner,
    const char *reponame,
    bool all,
    ghcli_issue **out)
{
    int                 count       = 0;
    json_stream         stream      = {0};
    ghcli_fetch_buffer  json_buffer = {0};
    char               *url         = NULL;

    url = sn_asprintf(
        "https://api.github.com/repos/%s/%s/issues?per_page=100&state=%s",
        owner, reponame,
        all ? "all" : "open");
    ghcli_fetch(url, &json_buffer);

    free(url);

    json_open_buffer(&stream, json_buffer.data, json_buffer.length);
    json_set_streaming(&stream, true);

    enum json_type next_token = json_next(&stream);

    while ((next_token = json_peek(&stream)) != JSON_ARRAY_END) {

        switch (next_token) {
        case JSON_ERROR:
            errx(1, "Parser error: %s", json_get_error(&stream));
            break;
        case JSON_OBJECT: {
            *out = realloc(*out, sizeof(ghcli_issue) * (count + 1));
            ghcli_issue *it = &(*out)[count];
            memset(it, 0, sizeof(ghcli_issue));
            parse_issue_entry(&stream, it);
            count += 1;
        } break;
        default:
            errx(1, "Unexpected json type in response");
            break;
        }

    }

    free(json_buffer.data);
    json_close(&stream);

    return count;
}

void
ghcli_print_issues_table(FILE *stream, ghcli_issue *issues, int issues_size)
{
    if (issues_size == 0) {
        fprintf(stream, "No issues\n");
        return;
    }

    fprintf(stream, "%6.6s  %7.7s  %-s\n", "NUMBER", "STATE", "TITLE");
    for (int i = 0; i < issues_size; ++i) {
        fprintf(
            stream, "%6d  %7.7s  %-s\n",
            issues[i].number,
            issues[i].state,
            issues[i].title);
    }
}

static void
ghcli_parse_issue_details(json_stream *input, ghcli_issue_details *out)
{
    enum json_type key_type, value_type;
    const char *key;

    json_next(input);

    while ((key_type = json_next(input)) == JSON_STRING) {
        size_t len;
        key = json_get_string(input, &len);

        if (strncmp("title", key, len) == 0)
            out->title = get_sv(input);
        else if (strncmp("state", key, len) == 0)
            out->state = get_sv(input);
        else if (strncmp("body", key, len) == 0)
            out->body = get_sv(input);
        else if (strncmp("created_at", key, len) == 0)
            out->created_at = get_sv(input);
        else if (strncmp("number", key, len) == 0)
            out->number = get_int(input);
        else if (strncmp("comments", key, len) == 0)
            out->comments = get_int(input);
        else if (strncmp("user", key, len) == 0)
            out->author = get_user_sv(input);
        else if (strncmp("locked", key, len) == 0)
            out->locked = get_bool(input);
        else if (strncmp("labels", key, len) == 0)
            out->labels_size = ghcli_read_label_list(input, &out->labels);
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
ghcli_print_issue_summary(FILE *out, ghcli_issue_details *it)
{
    fprintf(out,
            "   NUMBER : %d\n"
            "    TITLE : "SV_FMT"\n"
            "  CREATED : "SV_FMT"\n"
            "   AUTHOR : "SV_FMT"\n"
            "    STATE : "SV_FMT"\n"
            " COMMENTS : %d\n"
            "   LOCKED : %s\n"
            "   LABELS : ",
            it->number,
            SV_ARGS(it->title), SV_ARGS(it->created_at),
            SV_ARGS(it->author), SV_ARGS(it->state),
            it->comments, sn_bool_yesno(it->locked));

    if (it->labels_size) {
        fprintf(out, SV_FMT, SV_ARGS(it->labels[0]));

        for (size_t i = 1; i < it->labels_size; ++i)
            fprintf(out, ", "SV_FMT, SV_ARGS(it->labels[0]));
    } else {
        fprintf(out, "none");
    }
    fputs("\n\n", out);

    /* The API may not return a body if the user didn't put in any
     * comment */
    if (it->body.data) {
        pretty_print(it->body.data, 4, 80, out);
        fputc('\n', out);
    }
}

static void
ghcli_issue_details_free(ghcli_issue_details *it)
{
    free(it->title.data);
    free(it->created_at.data);
    free(it->author.data);
    free(it->state.data);
    free(it->body.data);

    for (size_t i = 0; i < it->labels_size; ++i)
        free(it->labels[i].data);

    free(it->labels);
}

void
ghcli_issue_summary(
    FILE *stream,
    const char *owner,
    const char *repo,
    int issue_number)
{
    const char          *url     = NULL;
    ghcli_fetch_buffer   buffer  = {0};
    json_stream          parser  = {0};
    ghcli_issue_details  details = {0};

    url = sn_asprintf(
        "https://api.github.com/repos/%s/%s/issues/%d?per_page=100",
        owner, repo,
        issue_number);
    ghcli_fetch(url, &buffer);

    json_open_buffer(&parser, buffer.data, buffer.length);
    json_set_streaming(&parser, true);

    ghcli_parse_issue_details(&parser, &details);
    ghcli_print_issue_summary(stream, &details);

    json_close(&parser);

    ghcli_issue_details_free(&details);
    free((void *)url);
    free(buffer.data);
}

void
ghcli_issue_close(const char *owner, const char *repo, int issue_number)
{
    ghcli_fetch_buffer  json_buffer = {0};
    const char         *url         = NULL;
    const char         *data        = NULL;

    url  = sn_asprintf(
        "https://api.github.com/repos/%s/%s/issues/%d",
        owner, repo,
        issue_number);
    data = sn_asprintf("{ \"state\": \"close\"}");

    ghcli_fetch_with_method("PATCH", url, data, &json_buffer);

    free((void *)data);
    free((void *)url);
    free(json_buffer.data);
}

void
ghcli_issue_reopen(const char *owner, const char *repo, int issue_number)
{
    ghcli_fetch_buffer  json_buffer = {0};
    const char         *url         = NULL;
    const char         *data        = NULL;

    url  = sn_asprintf(
        "https://api.github.com/repos/%s/%s/issues/%d",
        owner, repo,
        issue_number);
    data = sn_asprintf("{ \"state\": \"open\"}");

    ghcli_fetch_with_method("PATCH", url, data, &json_buffer);

    free((void *)data);
    free((void *)url);
    free(json_buffer.data);
}

static void
issue_init_user_file(FILE *stream, void *_opts)
{
    ghcli_submit_issue_options *opts = _opts;
    fprintf(
        stream,
        "# ISSUE TITLE : "SV_FMT"\n"
        "# Enter issue description below.\n"
        "# All lines starting with '#' will be discarded.\n",
        SV_ARGS(opts->title));
}

static sn_sv
ghcli_issue_get_user_message(ghcli_submit_issue_options *opts)
{
    return ghcli_editor_get_user_message(issue_init_user_file, opts);
}

void
ghcli_issue_submit(ghcli_submit_issue_options opts)
{
    ghcli_fetch_buffer  json_buffer  = {0};

    sn_sv body = ghcli_issue_get_user_message(&opts);

    opts.body = ghcli_json_escape(body);

    fprintf(stdout,
            "The following issue will be created:\n"
            "\n"
            "TITLE   : "SV_FMT"\n"
            "IN      : "SV_FMT"\n"
            "MESSAGE :\n"SV_FMT"\n",
            SV_ARGS(opts.title),SV_ARGS(opts.in),
            SV_ARGS(body));

    fputc('\n', stdout);

    if (!opts.always_yes) {
        if (!sn_yesno("Do you want to continue?"))
            errx(1, "Submission aborted.");
    }

    perform_submit_issue(opts, &json_buffer);
    ghcli_print_html_url(json_buffer);

    free(body.data);
    free(opts.body.data);
    free(json_buffer.data);
}
