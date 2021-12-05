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

#include <ghcli/releases.h>
#include <ghcli/json_util.h>
#include <pdjson/pdjson.h>

#include <assert.h>

static void
parse_release(struct json_stream *stream, ghcli_release *out)
{
    enum json_type  next       = JSON_NULL;
    enum json_type  value_type = JSON_NULL;
    const char     *key;

    if ((next = json_next(stream)) != JSON_OBJECT)
        errx(1, "Expected Release Object");

    while ((next = json_next(stream)) == JSON_STRING) {
        size_t len;
        key = json_get_string(stream, &len);

        if (strncmp("name", key, len) == 0) {
            out->name = get_sv(stream);
        } else if (strncmp("body", key, len) == 0) {
            out->body = get_sv(stream);
        } else if (strncmp("tarball_url", key, len) == 0) {
            out->tarball_url = get_sv(stream);
        } else if (strncmp("author", key, len) == 0) {
            out->author = get_user_sv(stream);
        } else if (strncmp("created_at", key, len) == 0) {
            out->date = get_sv(stream);
        } else if (strncmp("draft", key, len) == 0) {
            out->draft = get_bool(stream);
        } else if (strncmp("prerelease", key, len) == 0) {
            out->prerelease = get_bool(stream);
        } else if (strncmp("upload_url", key, len) == 0) {
            out->upload_url = get_sv(stream);
        } else if (strncmp("html_url", key, len) == 0) {
            out->html_url = get_sv(stream);
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
        errx(1, "Unclosed Release Object");
}

int
ghcli_get_releases(const char *owner, const char *repo, ghcli_release **out)
{
    char               *url    = NULL;
    ghcli_fetch_buffer  buffer = {0};
    struct json_stream  stream = {0};
    enum json_type      next   = JSON_NULL;
    int                 size   = 0;

    *out = NULL;

    url = sn_asprintf(
        "https://api.github.com/repos/%s/%s/releases",
        owner, repo);

    ghcli_fetch(url, &buffer);

    json_open_buffer(&stream, buffer.data, buffer.length);
    json_set_streaming(&stream, 1);

    if ((next = json_next(&stream)) != JSON_ARRAY)
        errx(1, "Expected array of releses");

    while ((next = json_peek(&stream)) == JSON_OBJECT) {
        *out = realloc(*out, sizeof(**out) * (size + 1));
        ghcli_release *it = &(*out)[size++];
        parse_release(&stream, it);
    }

    json_close(&stream);
    free(url);
    free(buffer.data);

    return size;
}

void
ghcli_print_releases(FILE *stream, ghcli_release *releases, int releases_size)
{
    if (releases_size == 0) {
        fprintf(stream, "No releases");
        return;
    }

    for (int i = 0; i < releases_size; ++i) {
        fprintf(stream,
                "      NAME : "SV_FMT"\n"
                "    AUTHOR : "SV_FMT"\n"
                "      DATE : "SV_FMT"\n"
                "     DRAFT : %s\n"
                "PRERELEASE : %s\n"
                "   TARBALL : "SV_FMT"\n"
                "      BODY :\n",
                SV_ARGS(releases[i].name),
                SV_ARGS(releases[i].author),
                SV_ARGS(releases[i].date),
                sn_bool_yesno(releases[i].draft),
                sn_bool_yesno(releases[i].prerelease),
                SV_ARGS(releases[i].tarball_url));

        pretty_print(releases[i].body.data, 13, 80, stream);

        fputc('\n', stream);
    }
}

void
ghcli_free_releases(ghcli_release *releases, int releases_size)
{
    assert(releases);

    for (int i = 0; i < releases_size; ++i) {
        free(releases[i].tarball_url.data);
        free(releases[i].name.data);
        free(releases[i].body.data);
        free(releases[i].author.data);
        free(releases[i].date.data);
    }

    free(releases);
}

static void
parse_single_release(ghcli_fetch_buffer buffer, ghcli_release *out)
{
    struct json_stream stream  = {0};

    json_open_buffer(&stream, buffer.data, buffer.length);
    json_set_streaming(&stream, 1);
    parse_release(&stream, out);
    json_close(&stream);
}

static char *
get_upload_url(ghcli_release *it)
{
    char *delim = strchr(it->upload_url.data, '{');
    if (delim == NULL)
        errx(1, "GitHub API returned an invalid upload url");

    size_t len = delim - it->upload_url.data;
    return sn_strndup(it->upload_url.data, len);
}

static void
upload_release_asset(const char *url, ghcli_release_asset asset)
{
    char               *req           = NULL;
    sn_sv               file_content  = {0};
    ghcli_fetch_buffer  buffer        = {0};

    file_content.length = sn_mmap_file(asset.path, (void **)&file_content.data);
    if (file_content.length == 0)
        errx(1, "Trying to upload an empty file");

    /* TODO: URL escape this */
    req = sn_asprintf("%s?name=%s", url, asset.name);

    ghcli_post_upload(
        req,
        "application/octet-stream", /* HACK */
        file_content.data,
        file_content.length,
        &buffer);

    free(req);
    free(buffer.data);
}

void
ghcli_create_release(const ghcli_new_release *release)
{
    char               *url            = NULL;
    char               *upload_url     = NULL;
    char               *post_data      = NULL;
    char               *name_json      = NULL;
    char               *commitish_json = NULL;
    sn_sv               escaped_body   = {0};
    ghcli_fetch_buffer  buffer         = {0};
    ghcli_release       response       = {0};

    assert(release);

    /* https://docs.github.com/en/rest/reference/repos#create-a-release */
    url = sn_asprintf(
        "https://api.github.com/repos/%s/%s/releases",
        release->owner,
        release->repo);

    escaped_body = ghcli_json_escape(release->body);

    if (release->commitish)
        commitish_json = sn_asprintf(
            ",\"target_commitish\": \"%s\"",
            release->commitish);

    if (release->name)
        name_json = sn_asprintf(
            ",\"name\": \"%s\"",
            release->name);

    post_data = sn_asprintf(
        "{"
        "    \"tag_name\": \"%s\","
        "    \"draft\": %s,"
        "    \"prerelease\": %s,"
        "    \"body\": \""SV_FMT"\""
        "    %s"
        "    %s"
        "}",
        release->tag,
        ghcli_json_bool(release->draft),
        ghcli_json_bool(release->prerelease),
        SV_ARGS(escaped_body),
        commitish_json ? commitish_json : "",
        name_json ? name_json : "");

    ghcli_fetch_with_method("POST", url, post_data, &buffer);
    parse_single_release(buffer, &response);

    printf("INFO : Release at "SV_FMT"\n", SV_ARGS(response.html_url));

    upload_url = get_upload_url(&response);

    for (size_t i = 0; i < release->assets_size; ++i) {
        printf("INFO : Uploading asset %s...\n", release->assets[i].path);
        upload_release_asset(upload_url, release->assets[i]);
    }

    free(upload_url);
    free(buffer.data);
    free(url);
    free(post_data);
    free(escaped_body.data);
    free(name_json);
    free(commitish_json);
}

void
ghcli_release_push_asset(ghcli_new_release *release, ghcli_release_asset asset)
{
    if (release->assets_size == GHCLI_RELEASE_MAX_ASSETS)
        errx(1, "Too many assets");

    release->assets[release->assets_size++] = asset;
}
