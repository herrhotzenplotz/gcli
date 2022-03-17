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
#include <ghcli/gitlab/forks.h>
#include <ghcli/json_util.h>

#include <pdjson/pdjson.h>

static sn_sv
parse_namespace(struct json_stream *input)
{
    enum json_type  key_type = JSON_NULL;
    sn_sv           result   = {0};
    const char     *key      = NULL;

    if (json_next(input) != JSON_OBJECT)
        errx(1, "Expected an object for a namespace");

    while ((key_type = json_next(input)) == JSON_STRING) {
        size_t len;
        key = json_get_string(input, &len);
        if (strncmp("full_path", key, len) == 0)
            result = get_sv(input);
        else
            json_next(input);
    }

    if (key_type != JSON_OBJECT_END)
        errx(1, "Namespace object not closed");

    return result;
}

static void
parse_fork(struct json_stream *input, ghcli_fork *out)
{
    enum json_type  key_type   = JSON_NULL;
    const char     *key        = NULL;

    if (json_next(input) != JSON_OBJECT)
        errx(1, "Expected an object for a fork");

    while ((key_type = json_next(input)) == JSON_STRING) {
        size_t len;
        key = json_get_string(input, &len);

        if (strncmp("path_with_namespace", key, len) == 0)
            out->full_name = get_sv(input);
        else if (strncmp("namespace", key, len) == 0)
            out->owner = parse_namespace(input);
        else if (strncmp("created_at", key, len) == 0)
            out->date = get_sv(input);
        else if (strncmp("forks_count", key, len) == 0)
            out->forks = get_int(input);
        else
            SKIP_OBJECT_VALUE(input);
    }

    if (key_type != JSON_OBJECT_END)
        errx(1, "Fork object not closed");
}

int
gitlab_get_forks(
    const char  *owner,
    const char  *repo,
    int          max,
    ghcli_fork **out)
{
    ghcli_fetch_buffer  buffer   = {0};
    char               *url      = NULL;
    char               *e_owner  = NULL;
    char               *e_repo   = NULL;
    char               *next_url = NULL;
    enum   json_type    next     = JSON_NULL;
    struct json_stream  stream   = {0};
    int                 size     = 0;

    e_owner = ghcli_urlencode(owner);
    e_repo  = ghcli_urlencode(repo);

    *out = NULL;

    url = sn_asprintf(
        "%s/projects/%s%%2F%s/forks",
        gitlab_get_apibase(),
        e_owner, e_repo);

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
    free(e_owner);
    free(e_repo);

    return size;
}

void
gitlab_fork_create(const char *owner, const char *repo, const char *_in)
{
    char               *url       = NULL;
    char               *e_owner   = NULL;
    char               *e_repo    = NULL;
    char               *post_data = NULL;
    sn_sv               in        = SV_NULL;
    ghcli_fetch_buffer  buffer    = {0};

    e_owner = ghcli_urlencode(owner);
    e_repo  = ghcli_urlencode(repo);

    url = sn_asprintf(
        "%s/projects/%s%%2F%s/fork",
        gitlab_get_apibase(),
        e_owner, e_repo);
    if (_in) {
        in        = ghcli_json_escape(SV((char *)_in));
        post_data = sn_asprintf("{\"namespace_path\":\""SV_FMT"\"}",
                                SV_ARGS(in));
    }

    ghcli_fetch_with_method("POST", url, post_data, NULL, &buffer);
    ghcli_print_html_url(buffer);

    free(in.data);
    free(url);
    free(post_data);
    free(e_owner);
    free(e_repo);
    free(buffer.data);
}
