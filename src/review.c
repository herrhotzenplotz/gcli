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

#include <ghcli/config.h>
#include <ghcli/curl.h>
#include <ghcli/github/config.h>
#include <ghcli/json_util.h>
#include <ghcli/review.h>

#include <pdjson/pdjson.h>

#include <limits.h>

static void
parse_review_comment(json_stream *stream, ghcli_pr_review_comment *it)
{
    if (json_next(stream) != JSON_OBJECT)
        errx(1, "Expected review comment object");

    enum json_type key_type;
    while ((key_type = json_next(stream)) == JSON_STRING) {
        size_t          len        = 0;
        const char     *key        = json_get_string(stream, &len);
        enum json_type  value_type = 0;

        if (strncmp("bodyText", key, len) == 0)
            it->body = get_string(stream);
        else if (strncmp("id", key, len) == 0)
            it->id = get_string(stream);
        else if (strncmp("createdAt", key, len) == 0)
            it->date = get_string(stream);
        else if (strncmp("author", key, len) == 0)
            it->author = get_user(stream);
        else if (strncmp("diffHunk", key, len) == 0)
            it->diff = get_string(stream);
        else if (strncmp("path", key, len) == 0)
            it->path = get_string(stream);
        else if (strncmp("originalPosition", key, len) == 0)
            it->original_position = get_int(stream);
        else {
            value_type = json_next(stream);

            switch (value_type) {
            case JSON_ARRAY:
                json_skip_until(stream, JSON_ARRAY_END);
                break;
            case JSON_OBJECT:
                json_skip_until(stream, JSON_OBJECT_END);
                break;
            default:
                break;
            }
        }
    }
}

static void
parse_review_comments(json_stream *stream, ghcli_pr_review *it)
{
    ghcli_json_advance(stream, "{s[", "nodes");
    while (json_peek(stream) == JSON_OBJECT) {
        it->comments = realloc(
            it->comments,
            sizeof(*it->comments) * (it->comments_size + 1));
        ghcli_pr_review_comment *comment = &it->comments[it->comments_size++];
        *comment                         = (ghcli_pr_review_comment) {0};
        parse_review_comment(stream, comment);
    }
    ghcli_json_advance(stream, "]}");
}

static void
parse_review_header(json_stream *stream, ghcli_pr_review *it)
{
    if (json_next(stream) != JSON_OBJECT)
        errx(1, "Expected review object");

    enum json_type key_type;
    while ((key_type = json_next(stream)) == JSON_STRING) {
        size_t          len        = 0;
        const char     *key        = json_get_string(stream, &len);
        enum json_type  value_type = 0;

        if (strncmp("bodyText", key, len) == 0)
            it->body = get_string(stream);
        else if (strncmp("state", key, len) == 0)
            it->state = get_string(stream);
        else if (strncmp("id", key, len) == 0)
            it->id = get_string(stream);
        else if (strncmp("createdAt", key, len) == 0)
            it->date = get_string(stream);
        else if (strncmp("author", key, len) == 0)
            it->author = get_user(stream);
        else if (strncmp("comments", key, len) == 0) {
            parse_review_comments(stream, it);
        } else {
            value_type = json_next(stream);

            switch (value_type) {
            case JSON_ARRAY:
                json_skip_until(stream, JSON_ARRAY_END);
                break;
            case JSON_OBJECT:
                json_skip_until(stream, JSON_OBJECT_END);
                break;
            default:
                break;
            }
        }
    }
}

static const char *get_reviews_fmt =
    "query {"
    "  repository(owner: \"%s\", name: \"%s\") {"
    "    pullRequest(number: %d) {"
    "      reviews(first: 10) {"
    "        nodes {"
    "          author {"
    "            login"
    "          }"
    "          bodyText"
    "          id"
    "          createdAt"
    "          state"
    "          comments(first: 10) {"
    "            nodes {"
    "              bodyText"
    "              diffHunk"
    "              path"
    "              originalPosition"
    "              author {"
    "                login"
    "              }"
    "              state"
    "              createdAt"
    "              id"
    "            }"
    "          }"
    "        }"
    "      }"
    "    }"
    "  }"
    "}";

size_t
ghcli_review_get_reviews(
    const char *owner,
    const char *repo,
    int pr,
    ghcli_pr_review **out)
{
    ghcli_fetch_buffer  buffer        = {0};
    char               *url           = NULL;
    char               *query         = NULL;
    sn_sv               query_escaped = {0};
    char               *post_data     = NULL;
    struct json_stream  stream        = {0};
    enum   json_type    next          = JSON_NULL;
    size_t              size          = 0;

    url           = sn_asprintf("%s/graphql", github_get_apibase());
    query         = sn_asprintf(get_reviews_fmt, owner, repo, pr);
    query_escaped = ghcli_json_escape(SV(query));
    post_data     = sn_asprintf("{\"query\": \""SV_FMT"\"}",
                                SV_ARGS(query_escaped));
    ghcli_fetch_with_method("POST", url, post_data, NULL, &buffer);

    json_open_buffer(&stream, buffer.data, buffer.length);
    json_set_streaming(&stream, true);

    ghcli_json_advance(
        &stream, "{s{s{s{s{s",
        "data", "repository", "pullRequest", "reviews", "nodes");

    next = json_next(&stream);
    if (next != JSON_ARRAY)
        errx(1, "error: expected json array for review list");

    while ((next = json_peek(&stream)) == JSON_OBJECT) {
        *out = realloc(*out, sizeof(ghcli_pr_review) * (size + 1));
        ghcli_pr_review *it = &(*out)[size];

        *it = (ghcli_pr_review) {0};

        parse_review_header(&stream, it);

        size++;
    }

    if (json_next(&stream) != JSON_ARRAY_END)
        errx(1, "error: expected end of json array");

    ghcli_json_advance(&stream, "}}}}}");

    free(buffer.data);
    free(url);
    free(query);
    free(query_escaped.data);
    free(post_data);
    json_close(&stream);

    return size;
}

void
ghcli_review_print_review_table(
    FILE *out,
    ghcli_pr_review *headers,
    size_t headers_size)
{
    for (size_t i = 0; i < headers_size; ++i) {
        fprintf(out,
                "ID     : %s\n"
                "AUTHOR : %s\n"
                "DATE   : %s\n"
                "STATE  : %s\n",
                headers[i].id, headers[i].author,
                headers[i].date, headers[i].state);

        pretty_print(headers[i].body, 9, 80, out);

        ghcli_review_print_comments(
            out,
            headers->comments,
            headers->comments_size);

        fputc('\n', out);
    }
}

void
ghcli_review_print_comments(
    FILE                    *out,
    ghcli_pr_review_comment *comments,
    size_t                   comments_size)
{
    for (size_t i = 0; i < comments_size; ++i) {
        fprintf(out,
                "       | PATH              : %s\n"
                "       | ORIGINAL POSITION : %d\n"
                "       | DIFF              :\n",
                comments[i].path,
                comments[i].original_position);

        pretty_print(comments[i].diff, 20, INT_MAX, out);

        fprintf(out, "       | MESSAGE :\n");
        pretty_print(comments[i].body, 20, 80, out);
    }
}

void
ghcli_review_reviews_free(ghcli_pr_review *it, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        free(it[i].author);
        free(it[i].date);
        free(it[i].state);
        free(it[i].body);
        free(it[i].id);
    }

    ghcli_review_comments_free(it->comments, it->comments_size);

    free(it);
}

void
ghcli_review_comments_free(ghcli_pr_review_comment *it, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        free(it[i].id);
        free(it[i].author);
        free(it[i].date);
        free(it[i].diff);
        free(it[i].path);
        free(it[i].body);
    }

    free(it);
}

size_t
ghcli_review_get_review_comments(
    const char               *owner,
    const char               *repo,
    int                       pr,
    int                       review_id,
    ghcli_pr_review_comment **out)
{
    char               *url    = NULL;
    ghcli_fetch_buffer  buffer = {0};
    struct json_stream  stream = {0};
    enum json_type      next   = JSON_NULL;
    size_t              size   = 0;

    url = sn_asprintf(
        "%s/repos/%s/%s/pulls/%d/reviews/%d/comments",
        github_get_apibase(),
        owner, repo, pr, review_id);
    ghcli_fetch(url, NULL, &buffer);

    json_open_buffer(&stream, buffer.data, buffer.length);
    json_set_streaming(&stream, true);

    next = json_next(&stream);
    if (next != JSON_ARRAY)
        errx(1, "error: expected json array for review comment list");

    while ((next = json_peek(&stream)) == JSON_OBJECT) {
        *out = realloc(*out, sizeof(ghcli_pr_review_comment) * (size + 1));

        /* Make sure, that we don't have garbarge uninitialized string
         * pointers in here */
        (*out)[size] = (ghcli_pr_review_comment) {0};
        parse_review_comment(&stream, &(*out)[size]);
        size++;
    }

    if (json_next(&stream) != JSON_ARRAY_END)
        errx(1, "error: expected end of json array");

    free(buffer.data);
    free(url);
    json_close(&stream);

    return size;
}
