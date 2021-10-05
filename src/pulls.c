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

#include <assert.h>
#include <string.h>

#include <sn/sn.h>
#include <ghcli/curl.h>
#include <ghcli/pulls.h>
#include <pdjson/pdjson.h>

static const char *
pull_get_user(json_stream *input)
{
    const char *result = NULL;
    while (json_next(input) == JSON_STRING) {
        size_t      len = 0;
        const char *key = json_get_string(input, &len);

        if (strncmp("login", key, len) == 0) {
            if (json_next(input) != JSON_STRING)
                errx(1, "login of the pull request creator is not a string");

            result = json_get_string(input, &len);
            result = sn_strndup(result, len);
        } else {
            json_next(input);
        }
    }

    return result;
}

static void
parse_pull_entry(json_stream *input, ghcli_pull *it)
{
    if (json_next(input) != JSON_OBJECT)
        errx(1, "Expected Issue Object");

    enum json_type key_type;
    while ((key_type = json_next(input)) == JSON_STRING) {
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
        } else if (strncmp("merged", key, len) == 0) {
            value_type = json_next(input);
            if (value_type == JSON_TRUE)
                it->merged = true;
            else if (value_type == JSON_FALSE)
                it->merged = false;
            else
                errx(1, "merged field is not a boolean value");
        } else if (strncmp("user", key, len) == 0) {
            if (json_next(input) != JSON_OBJECT)
                errx(1, "user field is not an object");

            it->creator = pull_get_user(input);
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
ghcli_get_pulls(const char *org, const char *reponame, ghcli_pull **out)
{
    int                 count       = 0;
    json_stream         stream      = {0};
    ghcli_fetch_buffer  json_buffer = {0};
    char               *url         = NULL;

    url = sn_asprintf("https://api.github.com/repos/%s/%s/pulls?per_page=100", org, reponame);
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
            *out = realloc(*out, sizeof(ghcli_pull) * (count + 1));
            ghcli_pull *it = &(*out)[count];
            memset(it, 0, sizeof(ghcli_pull));
            parse_pull_entry(&stream, it);
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
ghcli_print_pulls_table(FILE *stream, ghcli_pull *pulls, int pulls_size)
{
    fprintf(stream,     "%6s  %6s  %6s  %20s  %-s\n", "NUMBER", "STATE", "MERGED", "CREATOR", "TITLE");
    for (int i = 0; i < pulls_size; ++i) {
        fprintf(stream, "%6d  %6s  %6s  %20s  %-s\n",
                pulls[i].number,
                pulls[i].state,
                pulls[i].merged ? "yes" : "no",
                pulls[i].creator,
                pulls[i].title);
    }
}

void
ghcli_print_pull_diff(FILE *stream, const char *org, const char *reponame, int pr_number)
{
    char *url = NULL;

    url = sn_asprintf("https://api.github.com/repos/%s/%s/pulls/%d", org, reponame, pr_number);

    ghcli_curl(stream, url, "Accept: application/vnd.github.v3.diff");
}

static int
get_int(json_stream *input)
{
    if (json_next(input) != JSON_NUMBER)
        errx(1, "unexpected non-numeric field");

    return json_get_number(input);
}

static const char *
get_string(json_stream *input)
{
    if (json_next(input) != JSON_STRING)
        errx(1, "unexpected non-string field");

    size_t len;
    const char *it = json_get_string(input, &len);
    return sn_strndup(it, len);
}

static bool
get_bool(json_stream *input)
{
    enum json_type value_type = json_next(input);
    if (value_type == JSON_TRUE)
        return true;
    else if (value_type == JSON_FALSE)
        return false;
    else
        errx(1, "unexpected non-boolean value");
    assert(0 && "Not reached");
}

static void
ghcli_pull_parse_inspection(json_stream *input, ghcli_pull_inspection *out)
{
    enum json_type key_type, value_type;
    const char *key;

    json_next(input);

    while ((key_type = json_next(input)) == JSON_STRING) {
        size_t len;
        key = json_get_string(input, &len);

        if (strncmp("title", key, len) == 0)
            out->title = get_string(input);
        else if (strncmp("state", key, len) == 0)
            out->state = get_string(input);
        else if (strncmp("body", key, len) == 0)
            out->body = get_string(input);
        else if (strncmp("created_at", key, len) == 0)
            out->created_at = get_string(input);
        else if (strncmp("number", key, len) == 0)
            out->number = get_int(input);
        else if (strncmp("id", key, len) == 0)
            out->id = get_int(input);
        else if (strncmp("commits", key, len) == 0)
            out->commits = get_int(input);
        else if (strncmp("comments", key, len) == 0)
            out->comments = get_int(input);
        else if (strncmp("additions", key, len) == 0)
            out->additions = get_int(input);
        else if (strncmp("deletions", key, len) == 0)
            out->deletions = get_int(input);
        else if (strncmp("changed_files", key, len) == 0)
            out->changed_files = get_int(input);
        else if (strncmp("merged", key, len) == 0)
            out->merged = get_bool(input);
        else if (strncmp("mergeable", key, len) == 0)
            out->mergeable = get_bool(input);
        else if (strncmp("draft", key, len) == 0)
            out->draft = get_bool(input);
        else if (strncmp("user", key, len) == 0) {
            if (json_next(input) != JSON_OBJECT)
                errx(1, "user field is not an object");

            out->author = pull_get_user(input);
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
        errx(1, "Unexpected object key type");
}

static const char *
yesno(bool x)
{
    return x ? "yes" : "no";
}

static void
ghcli_print_pull_inspect_summary(FILE *out, ghcli_pull_inspection *it)
{
    fprintf(out,
            "   NUMBER : %d\n"
            "    TITLE : %s\n"
            "  CREATED : %s\n"
            "   AUTHOR : %s\n"
            "    STATE : %s\n"
            " COMMENTS : %d\n"
            "  ADD:DEL : %d:%d\n"
            "  COMMITS : %d\n"
            "  CHANGED : %d\n"
            "   MERGED : %s\n"
            "MERGEABLE : %s\n"
            "    DRAFT : %s\n",
            it->number, it->title, it->created_at, it->author, it->state, it->comments,
            it->additions, it->deletions, it->commits, it->changed_files,
            yesno(it->merged),
            yesno(it->mergeable),
            yesno(it->draft));
}

void
ghcli_inspect_pull(FILE *out, const char *org, const char *reponame, int pr_number)
{
    json_stream            stream      = {0};
    ghcli_fetch_buffer     json_buffer = {0};
    char                  *url         = NULL;
    ghcli_pull_inspection  result      = {0};

    url = sn_asprintf("https://api.github.com/repos/%s/%s/pulls/%d", org, reponame, pr_number);
    ghcli_fetch(url, &json_buffer);

    json_open_buffer(&stream, json_buffer.data, json_buffer.length);
    json_set_streaming(&stream, true);

    // fwrite(json_buffer.data, 1, json_buffer.length, out);
    ghcli_pull_parse_inspection(&stream, &result);
    ghcli_print_pull_inspect_summary(out, &result);
}
