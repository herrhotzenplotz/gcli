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

#include <ghcli/curl.h>
#include <ghcli/gitlab/config.h>
#include <ghcli/json_util.h>
#include <ghcli/snippets.h>

#include <pdjson/pdjson.h>
#include <sn/sn.h>

#include <stdlib.h>

void
ghcli_snippets_free(
    ghcli_snippet *list,
    int            list_size)
{
    for (int i = 0; i < list_size; ++i) {
        free(list[i].title);
        free(list[i].filename);
        free(list[i].date);
        free(list[i].author);
        free(list[i].visibility);
        free(list[i].raw_url);
    }

    free(list);
}

static void
gitlab_parse_snippet(struct json_stream *stream, ghcli_snippet *out)
{
    enum json_type  next       = JSON_NULL;
    enum json_type  value_type = JSON_NULL;
    const char     *key        = NULL;

    if ((next = json_next(stream)) != JSON_OBJECT)
        errx(1, "Expected snippet object");

    while ((next = json_next(stream)) == JSON_STRING) {
        size_t len;
        key = json_get_string(stream, &len);

        if (strncmp("title", key, len) == 0) {
            out->title = get_string(stream);
        } else if (strncmp("id", key, len) == 0) {
            out->id = get_int(stream);
        } else if (strncmp("raw_url", key, len) == 0) {
            out->raw_url = get_string(stream);
        } else if (strncmp("created_at", key, len) == 0) {
            out->date = get_string(stream);
        } else if (strncmp("file_name", key, len) == 0) {
            out->filename = get_string(stream);
        } else if (strncmp("author", key, len) == 0) {
            out->author = get_user(stream);
        } else if (strncmp("visibility", key, len) == 0) {
            out->visibility = get_string(stream);
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

    if (next != JSON_OBJECT_END)
        errx(1, "Unclosed snippet object");
}

int
ghcli_snippets_get(int max, ghcli_snippet **out)
{
    char               *url      = NULL;
    char               *next_url = NULL;
    ghcli_fetch_buffer  buffer   = {0};
    struct json_stream  stream   = {0};
    int                 size     = 0;

    *out = NULL;

    url = sn_asprintf("%s/snippets", gitlab_get_apibase());

    do {
        ghcli_fetch(url, &next_url, &buffer);

        json_open_buffer(&stream, buffer.data, buffer.length);
        json_set_streaming(&stream, 1);

        if (json_next(&stream) != JSON_ARRAY)
            errx(1, "Expected array");

        while (json_peek(&stream) == JSON_OBJECT) {
            *out = realloc(*out, sizeof(**out) * (size + 1));
            ghcli_snippet *it = &(*out)[size++];
            gitlab_parse_snippet(&stream, it);

            if (size == max)
                goto enough;
        }

        if (json_next(&stream) != JSON_ARRAY_END)
            errx(1, "Expected end of array");

    enough:
        json_close(&stream);
        free(url);
        free(buffer.data);
    } while ((url = next_url) && (max == -1 || size < max));

    free(next_url);

    return size;
}

static void
ghcli_print_snippet(FILE *stream, ghcli_snippet *it)
{
    fprintf(stream, "    ID : %d\n", it->id);
    fprintf(stream, " TITLE : %s\n", it->title);
    fprintf(stream, "AUTHOR : %s\n", it->author);
    fprintf(stream, "  FILE : %s\n", it->filename);
    fprintf(stream, "  DATE : %s\n", it->date);
    fprintf(stream, "VSBLTY : %s\n", it->visibility);
    fprintf(stream, "   URL : %s\n", it->raw_url);
}

void
ghcli_snippets_print(
    FILE                    *stream,
    enum ghcli_output_order  order,
    ghcli_snippet           *list,
    int                      list_size)
{
    if (list_size == 0) {
        fprintf(stream, "No Snippets\n");
        return;
    }

    /* output in reverse order if the sorted flag was enabled */
    if (order == OUTPUT_ORDER_SORTED) {
        for (int i = list_size; i > 0; --i) {
            ghcli_print_snippet(stream, &list[i - 1]);
            fputc('\n', stream);
        }
    } else {
        for (int i = 0; i < list_size; ++i) {
            ghcli_print_snippet(stream, &list[i]);
            fputc('\n', stream);
        }
    }
}

void
ghcli_snippet_delete(const char *snippet_id)
{
    ghcli_fetch_buffer buffer = {0};
    char *url = sn_asprintf("%s/snippets/%s", gitlab_get_apibase(), snippet_id);

    ghcli_fetch_with_method("DELETE", url, NULL, NULL, &buffer);

    free(url);
    free(buffer.data);
}

void
ghcli_snippet_get(const char *snippet_id)
{
    char *url = sn_asprintf("%s/snippets/%s/raw",
                            gitlab_get_apibase(),
                            snippet_id);
    ghcli_curl(stdout, url, NULL);
    free(url);
}
