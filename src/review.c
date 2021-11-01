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

#include <ghcli/review.h>
#include <ghcli/curl.h>
#include <ghcli/json_util.h>

#include <pdjson/pdjson.h>

static void
parse_review_header(json_stream *stream, ghcli_pr_review_header *it)
{
    if (json_next(stream) != JSON_OBJECT)
        errx(1, "Expected review object");

    enum json_type key_type;
    while ((key_type = json_next(stream)) == JSON_STRING) {
        size_t          len        = 0;
        const char     *key        = json_get_string(stream, &len);
        enum json_type  value_type = 0;

        if (strncmp("body", key, len) == 0)
            it->body = get_string(stream);
        else if (strncmp("state", key, len) == 0)
            it->state = get_string(stream);
        else if (strncmp("id", key, len) == 0)
            it->id = get_int(stream);
        else if (strncmp("submitted_at", key, len) == 0)
            it->date = get_string(stream);
        else if (strncmp("user", key, len) == 0)
            it->author = get_user(stream);
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

size_t
ghcli_review_get_reviews(const char *org, const char *repo, int pr, ghcli_pr_review_header **out)
{
    ghcli_fetch_buffer  buffer = {0};
    const char         *url    = NULL;
    struct json_stream  stream = {0};
    enum   json_type    next   = JSON_NULL;
    size_t              size   = 0;

    url = sn_asprintf("https://api.github.com/repos/%s/%s/pulls/%d/reviews", org, repo, pr);
    ghcli_fetch(url, &buffer);

    json_open_buffer(&stream, buffer.data, buffer.length);
    json_set_streaming(&stream, true);

    next = json_next(&stream);
    if (next != JSON_ARRAY)
        errx(1, "error: expected json array for review list");

    while ((next = json_peek(&stream)) == JSON_OBJECT) {
        *out = realloc(*out, sizeof(ghcli_pr_review_header) * size);
        parse_review_header(&stream, &(*out)[size]);
        size++;
    }

    if (json_next(&stream) != JSON_ARRAY_END)
        errx(1, "error: expected end of json array");

    return size;
}

void
ghcli_review_print_review_table(FILE *out, ghcli_pr_review_header *headers, size_t headers_size)
{
    for (size_t i = 0; i < headers_size; ++i) {
        fprintf(out,
                "ID     : %d\n"
                "AUTHOR : %s\n"
                "DATE   : %s\n"
                "STATE  : %s\n",
                headers[i].id, headers[i].author,
                headers[i].date, headers[i].state);

        pretty_print(headers[i].body, 9, 80, out);
        fputc('\n', out);
    }
}
