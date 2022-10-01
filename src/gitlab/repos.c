/*
 * Copyright 2021, 2022 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/gitlab/config.h>
#include <gcli/gitlab/repos.h>
#include <gcli/json_util.h>

#include <pdjson/pdjson.h>
#include <sn/sn.h>

static void
gitlab_parse_repo(json_stream *input, gcli_repo *out)
{
    enum json_type  next     = JSON_NULL;
    enum json_type  key_type = JSON_NULL;
    const char     *key      = NULL;

    if ((next = json_next(input)) != JSON_OBJECT)
        errx(1, "Expected an object for a repo");

    while ((key_type = json_next(input)) == JSON_STRING) {
        size_t len;
        key = json_get_string(input, &len);

        if (strncmp("path_with_namespace", key, len) == 0)
            out->full_name = get_sv(input);
        else if (strncmp("name", key, len) == 0)
            out->name = get_sv(input);
        else if (strncmp("owner", key, len) == 0)
            out->owner = get_user_sv(input);
        else if (strncmp("created_at", key, len) == 0)
            out->date = get_sv(input);
        else if (strncmp("visibility", key, len) == 0)
            out->visibility = get_sv(input);
        else if (strncmp("fork", key, len) == 0)
            out->is_fork = get_bool(input);
        else if (strncmp("id", key, len) == 0)
            out->id = get_int(input);
        else
            SKIP_OBJECT_VALUE(input);
    }

    if (key_type != JSON_OBJECT_END)
        errx(1, "Repo object not closed");
}

void
gitlab_get_repo(
    sn_sv      owner,
    sn_sv      repo,
    gcli_repo *out)
{
    /* GET /projects/:id */
    char               *url     = NULL;
    gcli_fetch_buffer   buffer  = {0};
    struct json_stream  stream  = {0};
    sn_sv               e_owner = {0};
    sn_sv               e_repo  = {0};

    e_owner = gcli_urlencode_sv(owner);
    e_repo  = gcli_urlencode_sv(repo);

    url = sn_asprintf(
        "%s/projects/"SV_FMT"%%2F"SV_FMT,
        gitlab_get_apibase(),
        SV_ARGS(e_owner), SV_ARGS(e_repo));

    gcli_fetch(url, NULL, &buffer);
    json_open_buffer(&stream, buffer.data, buffer.length);

    gitlab_parse_repo(&stream, out);

    json_close(&stream);
    free(buffer.data);
    free(e_owner.data);
    free(e_repo.data);
    free(url);
}

int
gitlab_get_repos(
    const char  *owner,
    int          max,
    gcli_repo  **out)
{
    char               *url      = NULL;
    char               *next_url = NULL;
    char               *e_owner  = NULL;
    gcli_fetch_buffer   buffer   = {0};
    struct json_stream  stream   = {0};
    enum  json_type     next     = JSON_NULL;
    int                 size     = 0;

    e_owner = gcli_urlencode(owner);

    url = sn_asprintf("%s/users/%s/projects", gitlab_get_apibase(), e_owner);

    do {
        gcli_fetch(url, &next_url, &buffer);

        json_open_buffer(&stream, buffer.data, buffer.length);
        json_set_streaming(&stream, 1);

        // TODO: Poor error message
        if ((next = json_next(&stream)) != JSON_ARRAY)
            errx(1,
                 "Expected array in response from API "
                 "but got something else instead");

        while ((next = json_peek(&stream)) != JSON_ARRAY_END) {
            *out = realloc(*out, sizeof(**out) * (size + 1));
            gcli_repo *it = &(*out)[size++];
            gitlab_parse_repo(&stream, it);

            if (size == max)
                break;
        }

        free(url);
        free(buffer.data);
        json_close(&stream);
    } while ((url = next_url) && (max == -1 || size < max));

    free(url);
    free(e_owner);

    return size;
}

int
gitlab_get_own_repos(
    int         max,
    gcli_repo **out)
{
    char  *_account = NULL;
    sn_sv  account  = {0};
    int    n;

    account = gitlab_get_account();
    if (!account.length)
        errx(1, "error: gitlab.account is not set");

    _account = sn_sv_to_cstr(account);

    n = gitlab_get_repos(_account, max, out);

    free(_account);

    return n;
}

void
gitlab_repo_delete(
    const char *owner,
    const char *repo)
{
    char              *url     = NULL;
    char              *e_owner = NULL;
    char              *e_repo  = NULL;
    gcli_fetch_buffer  buffer  = {0};

    e_owner = gcli_urlencode(owner);
    e_repo  = gcli_urlencode(repo);

    url = sn_asprintf("%s/projects/%s%%2F%s",
                      gitlab_get_apibase(),
                      e_owner, e_repo);

    gcli_fetch_with_method("DELETE", url, NULL, NULL, &buffer);

    free(buffer.data);
    free(url);
    free(e_owner);
    free(e_repo);
}

gcli_repo *
gitlab_repo_create(
    const gcli_repo_create_options *options) /* Options descriptor */
{
    gcli_repo          *repo;
    char               *url, *data;
    gcli_fetch_buffer   buffer = {0};
    struct json_stream  stream = {0};

    /* Will be freed by the caller with gcli_repos_free */
    repo = calloc(1, sizeof(gcli_repo));

    /* Request preparation */
    url = sn_asprintf("%s/projects", gitlab_get_apibase());
    /* TODO: escape the repo name and the description */
    data = sn_asprintf("{\"name\": \""SV_FMT"\","
                       " \"description\": \""SV_FMT"\","
                       " \"visibility\": \"%s\" }",
                       SV_ARGS(options->name),
                       SV_ARGS(options->description),
                       options->private ? "private" : "public");

    /* Fetch and parse result */
    gcli_fetch_with_method("POST", url, data, NULL, &buffer);
    json_open_buffer(&stream, buffer.data, buffer.length);
    gitlab_parse_repo(&stream, repo);

    /* Cleanup */
    json_close(&stream);
    free(buffer.data);
    free(data);
    free(url);

    return repo;
}
