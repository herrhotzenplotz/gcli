/*
 * Copyright 2022 Nico Sonack <nsonack@outlook.com>
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

#include <ghcli/gitlab/config.h>
#include <ghcli/gitlab/labels.h>
#include <ghcli/json_util.h>

#include <pdjson/pdjson.h>

static uint32_t
gitlab_get_color(struct json_stream *input)
{
    char *color  = get_string(input);
    char *endptr = NULL;
    long  code   = 0;

    /* Skip # */
    color += 1;

    code = strtol(color, &endptr, 16);
    if (endptr != color + strlen(color))
        err(1, "error: color code is invalid");

    return ((uint32_t)(code) << 8);
}

static void
gitlab_parse_label(struct json_stream *input, ghcli_label *it)
{
    if (json_next(input) != JSON_OBJECT)
        errx(1, "expected label object");

    while (json_next(input) == JSON_STRING) {
        size_t      len = 0;
        const char *key = json_get_string(input, &len);

        if (strncmp("name", key, len) == 0)
            it->name = get_string(input);
        else if (strncmp("description", key, len) == 0)
            it->description = get_string(input);
        else if (strncmp("color", key, len) == 0)
            it->color = gitlab_get_color(input);
        else if (strncmp("id", key, len) == 0)
            it->id = get_int(input);
        else
            SKIP_OBJECT_VALUE(input);
    }
}

size_t
gitlab_get_labels(
    const char   *owner,
    const char   *repo,
    int           max,
    ghcli_label **out)
{
    size_t              out_size = 0;
    char               *url      = NULL;
    char               *next_url = NULL;
    ghcli_fetch_buffer  buffer   = {0};
    struct json_stream  stream   = {0};

    *out = NULL;

    url = sn_asprintf("%s/projects/%s%%2F%s/labels",
                      gitlab_get_apibase(), owner, repo);

    do {
        enum json_type next_type;

        ghcli_fetch(url, &next_url, &buffer);

        json_open_buffer(&stream, buffer.data, buffer.length);
        json_set_streaming(&stream, 1);

        next_type = json_next(&stream);
        if (next_type != JSON_ARRAY)
            errx(1, "error: expected array of labels");

        while (json_peek(&stream) != JSON_ARRAY_END) {
            ghcli_label *it;

            *out = realloc(*out, sizeof(ghcli_label) * (out_size + 1));
            it = &(*out)[out_size++];
            gitlab_parse_label(&stream, it);
        }

        free(buffer.data);
        free(url);
        json_close(&stream);
    } while ((url = next_url) && (max == -1 || (int)out_size < max));

    return out_size;
}
