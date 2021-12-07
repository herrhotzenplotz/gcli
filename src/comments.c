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

#include <ghcli/comments.h>
#include <ghcli/curl.h>
#include <ghcli/editor.h>
#include <ghcli/json_util.h>
#include <pdjson/pdjson.h>
#include <sn/sn.h>

#include <ctype.h>
#include <string.h>


static void
perform_submit_comment(
    ghcli_submit_comment_opts opts,
    ghcli_fetch_buffer *out)
{
    char *post_fields = sn_asprintf(
        "{ \"body\": \""SV_FMT"\" }",
        SV_ARGS(opts.message));
    char *url         = sn_asprintf(
        "https://api.github.com/repos/%s/%s/issues/%d/comments",
        opts.owner, opts.repo, opts.issue);

    ghcli_fetch_with_method("POST", url, post_fields, NULL, out);
    free(post_fields);
    free(url);
}

static void
ghcli_parse_comment(json_stream *input, ghcli_comment *it)
{
    if (json_next(input) != JSON_OBJECT)
        errx(1, "Expected Comment Object");

    enum json_type key_type;
    while ((key_type = json_next(input)) == JSON_STRING) {
        size_t          len        = 0;
        const char     *key        = json_get_string(input, &len);
        enum json_type  value_type = 0;

        if (strncmp("created_at", key, len) == 0)
            it->date = get_string(input);
        else if (strncmp("body", key, len) == 0)
            it->body = get_string(input);
        else if (strncmp("user", key, len) == 0)
            it->author = get_user(input);
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
ghcli_get_comments(const char *url, ghcli_comment **comments)
{
    int                count       = 0;
    json_stream        stream      = {0};
    ghcli_fetch_buffer json_buffer = {0};

    ghcli_fetch(url, NULL, &json_buffer);
    json_open_buffer(&stream, json_buffer.data, json_buffer.length);
    json_set_streaming(&stream, true);

    enum json_type next_token = json_next(&stream);

    while ((next_token = json_peek(&stream)) != JSON_ARRAY_END) {
        if (next_token != JSON_OBJECT)
            errx(1, "Unexpected non-object in comment list");

        *comments = realloc(*comments, (count + 1) * sizeof(ghcli_comment));
        ghcli_comment *it = &(*comments)[count];
        ghcli_parse_comment(&stream, it);
        count += 1;
    }

    json_close(&stream);
    free(json_buffer.data);

    return count;
}

static void
ghcli_issue_comment_free(ghcli_comment *it)
{
    free((void *)it->author);
    free((void *)it->date);
    free((void *)it->body);
}

void
ghcli_print_comment_list(
    FILE *stream,
    ghcli_comment *comments,
    size_t comments_size)
{
    for (size_t i = 0; i < comments_size; ++i) {
        fprintf(stream,
                "AUTHOR : %s\n"
                "DATE   : %s\n", comments[i].author, comments[i].date);
        pretty_print(comments[i].body, 9, 80, stream);
        fputc('\n', stream);
    }
}

void
ghcli_issue_comments(FILE *stream, const char *owner, const char *repo, int issue)
{
    const char    *url      = sn_asprintf(
        "https://api.github.com/repos/%s/%s/issues/%d/comments",
        owner, repo, issue);
    ghcli_comment *comments = NULL;
    int            n        = ghcli_get_comments(url, &comments);
    ghcli_print_comment_list(stream, comments, (size_t)n);

    for (int i = 0; i < n; ++i)
        ghcli_issue_comment_free(&comments[i]);

    free((void *)url);
    free(comments);
}

static void
comment_init(FILE *f, void *_data)
{
    ghcli_submit_comment_opts *info = _data;

    fprintf(
        f,
        "# Enter your comment below, save and exit.\n"
        "# All lines with a leading '#' are discarded and will not\n"
        "# appear in your comment.\n"
        "# COMMENT IN : %s/%s #%d\n",
        info->owner, info->repo, info->issue);
}

static sn_sv
ghcli_comment_get_message(ghcli_submit_comment_opts *info)
{
    return ghcli_editor_get_user_message(comment_init, info);
}

void
ghcli_comment_submit(ghcli_submit_comment_opts opts)
{
    ghcli_fetch_buffer buffer = {0};
    sn_sv message = ghcli_comment_get_message(&opts);
    opts.message  = ghcli_json_escape(message);

    fprintf(
        stdout,
        "You will be commenting the following in %s/%s #%d:\n"SV_FMT"\n",
        opts.owner, opts.repo, opts.issue, SV_ARGS(message));

    if (!opts.always_yes) {
        if (!sn_yesno("Is this okay?"))
            errx(1, "Aborted by user");
    }

    perform_submit_comment(opts, &buffer);
    ghcli_print_html_url(buffer);

    free(buffer.data);
    free(message.data);
    free(opts.message.data);
}
