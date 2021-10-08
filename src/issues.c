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
#include <ghcli/issues.h>
#include <pdjson/pdjson.h>

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
            value_type = json_next(input);
            if (value_type != JSON_STRING)
                errx(1, "Title is not a string");

            const char *title = json_get_string(input, &len);
            it->title = sn_strndup(title, len);
        } else if (strncmp("state", key, len) == 0) {
            value_type = json_next(input);
            if (value_type != JSON_STRING)
                errx(1, "state is not a string");

            const char *state = json_get_string(input, &len);
            it->state = sn_strndup(state, len);
        } else if (strncmp("number", key, len) == 0) {
            value_type = json_next(input);
            if (value_type != JSON_NUMBER)
                errx(1, "number-field is not a number");

            it->number = json_get_number(input);
        } else if (strncmp("id", key, len) == 0) {
            value_type = json_next(input);
            if (value_type != JSON_NUMBER)
                errx(1, "id-field is not a number");

            it->id = json_get_number(input);
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

int
ghcli_get_issues(const char *org, const char *reponame, bool all, ghcli_issue **out)
{
    int                 count       = 0;
    json_stream         stream      = {0};
    ghcli_fetch_buffer  json_buffer = {0};
    char               *url         = NULL;

    url = sn_asprintf("https://api.github.com/repos/%s/%s/issues?per_page=100&state=%s", org, reponame,
                      all ? "all" : "open");
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

    return count;
}

void
ghcli_print_issues_table(FILE *stream, ghcli_issue *issues, int issues_size)
{
    fprintf(stream, "%5s  %7s  %s\n", "NUMBER", "STATE", "TITLE");
    for (int i = 0; i < issues_size; ++i) {
        fprintf(stream, "%5d  %7s  %s\n", issues[i].number, issues[i].state, issues[i].title);
    }
}
