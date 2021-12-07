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
#include <ghcli/forks.h>
#include <ghcli/json_util.h>

#include <pdjson/pdjson.h>

static void
parse_fork(struct json_stream *input, ghcli_fork *out)
{
    enum json_type  key_type   = JSON_NULL;
    enum json_type  value_type = JSON_NULL;
    const char     *key        = NULL;

    if (json_next(input) != JSON_OBJECT)
        errx(1, "Expected an object for a fork");

    while ((key_type = json_next(input)) == JSON_STRING) {
        size_t len;
        key = json_get_string(input, &len);

        if (strncmp("full_name", key, len) == 0) {
            out->full_name = get_sv(input);
        } else if (strncmp("owner", key, len) == 0) {
            char *user = get_user(input);
            out->owner = SV(user);
        } else if (strncmp("created_at", key, len) == 0) {
            out->date = get_sv(input);
        } else if (strncmp("forks_count", key, len) == 0) {
            out->forks = get_int(input);
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

    if (key_type != JSON_OBJECT_END)
        errx(1, "Fork object not closed");
}

int
ghcli_get_forks(
    const char  *owner,
    const char  *reponame,
    int          max,
    ghcli_fork **out)
{
    ghcli_fetch_buffer  buffer   = {0};
    char               *url      = NULL;
    char               *next_url = NULL;
    enum   json_type    next     = JSON_NULL;
    struct json_stream  stream   = {0};
    int                 size     = 0;

    *out = NULL;

    url = sn_asprintf(
        "%s/repos/%s/%s/forks",
        ghcli_config_get_apibase(),
        owner, reponame);

    do {
        ghcli_fetch(url, &next_url, &buffer);

        json_open_buffer(&stream, buffer.data, buffer.length);
        json_set_streaming(&stream, 1);

        // TODO: Poor error message
        if ((next = json_next(&stream)) != JSON_ARRAY)
            errx(1,
                 "Expected array in response from API "
                 "but got something else instead");

        while ((next = json_peek(&stream)) != JSON_ARRAY_END) {
            *out = realloc(*out, sizeof(ghcli_fork) * (size + 1));
            ghcli_fork *it = &(*out)[size++];
            parse_fork(&stream, it);

            if (size == max)
                break;
        }

        json_close(&stream);
        free(buffer.data);
        free(url);
    } while ((url = next_url) && (max == -1 || size < max));

    free(next_url);

    return size;
}

void
ghcli_print_forks(FILE *stream, ghcli_fork *forks, size_t forks_size)
{
    if (forks_size == 0) {
        fprintf(stream, "No forks\n");
        return;
    }

    fprintf(
        stream,
        "%-20.20s  %-20.20s  %-5.5s  %s\n",
        "OWNER", "DATE", "FORKS", "FULLNAME");
    for (size_t i = 0; i < forks_size; ++i) {
        fprintf(
            stream, "%-20.20s  %-20.20s  %5d  %s\n",
            forks[i].owner.data,
            forks[i].date.data,
            forks[i].forks,
            forks[i].full_name.data);
    }
}

void
ghcli_fork_create(const char *owner, const char *repo, const char *_in)
{
    char               *url       = NULL;
    char               *post_data = NULL;
    sn_sv               in        = SV_NULL;
    ghcli_fetch_buffer  buffer    = {0};

    url = sn_asprintf(
        "%s/repos/%s/%s/forks",
        ghcli_config_get_apibase(),
        owner, repo);
    if (_in) {
        in        = ghcli_json_escape(SV((char *)_in));
        post_data = sn_asprintf("{\"organization\":\""SV_FMT"\"}",
                                SV_ARGS(in));
    }

    ghcli_fetch_with_method("POST", url, post_data, NULL, &buffer);
    ghcli_print_html_url(buffer);

    free(in.data);
    free(url);
    free(post_data);
    free(buffer.data);
}
