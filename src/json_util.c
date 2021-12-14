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

#include <ghcli/json_util.h>
#include <ghcli/forges.h>
#include <sn/sn.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>

static inline void
barf(const char *message, const char *where)
{
    errx(1,
         "error: %s.\n"
         "       This might be an error on the GitHub api or the result of\n"
         "       incorrect usage of cli flags. See ghcli(1) to make sure your\n"
         "       flags are correct. If you are certain that all your options\n"
         "       were correct, please submit a bug report including the\n"
         "       command you invoked and the following information about the\n"
         "       error location: file = %s", message, where);
}

int
get_int_(json_stream *input, const char *where)
{
    if (json_next(input) != JSON_NUMBER)
        barf("unexpected non-numeric field", where);

    return json_get_number(input);
}

char *
get_string_(json_stream *input, const char *where)
{
    enum json_type type = json_next(input);
    if (type == JSON_NULL)
        return strdup("<empty>");

    if (type != JSON_STRING)
        barf("unexpected non-string field", where);

    size_t len;
    const char *it = json_get_string(input, &len);
    return sn_strndup(it, len);
}

bool
get_bool_(json_stream *input, const char *where)
{
    enum json_type value_type = json_next(input);
    if (value_type == JSON_TRUE)
        return true;
    else if (value_type == JSON_FALSE || value_type == JSON_NULL) // HACK
        return false;
    else
        barf("unexpected non-boolean value", where);

    errx(42, "%s: unreachable", where);
    return false;
}

char *
get_user_(json_stream *input, const char *where)
{
    if (json_next(input) != JSON_OBJECT)
        barf("user field is not an object", where);

    const char *expected_key = ghcli_forge()->user_object_key;

    char *result = NULL;
    while (json_next(input) == JSON_STRING) {
        size_t      len = 0;
        const char *key = json_get_string(input, &len);

        if (strncmp(expected_key, key, len) == 0) {
            if (json_next(input) != JSON_STRING)
                barf(
                    "login of the pull request creator is not a string",
                    where);

            const char *tmp = json_get_string(input, &len);
            result = sn_strndup(tmp, len);
        } else {
            json_next(input);
        }
    }

    return result;
}

static struct {
    char        c;
    const char *with;
} json_escape_table[] = {
    { .c = '\n', .with = "\\n"  },
    { .c = '\t', .with = "\\t"  },
    { .c = '\\', .with = "\\\\" },
    { .c = '"' , .with = "\\\"" },
};

sn_sv
ghcli_json_escape(sn_sv it)
{
    // TODO: Improve performance of ghcli_json_escape
    sn_sv result = {0};

    for (size_t i = 0; i < it.length; ++i) {
        for (size_t c = 0; c < ARRAY_SIZE(json_escape_table); ++c) {
            if (json_escape_table[c].c == it.data[i]) {
                size_t len     = strlen(json_escape_table[c].with);
                result.data    = realloc(result.data, result.length + len);
                memcpy(result.data + result.length,
                       json_escape_table[c].with,
                       len);
                result.length += len;
                goto next;
            }
        }

        result.data = realloc(result.data, result.length + 1);
        memcpy(result.data + result.length, it.data + i, 1);
        result.length += 1;
    next:
        continue;
    }

    return result;
}

sn_sv
get_sv_(json_stream *input, const char *where)
{
    enum json_type type = json_next(input);
    if (type == JSON_NULL)
        return SV_NULL;

    if (type != JSON_STRING)
        barf("unexpected non-string field", where);

    size_t len;
    const char *it   = json_get_string(input, &len);
    char       *copy = sn_strndup(it, len);
    return SV(copy);
}

void
ghcli_print_html_url(ghcli_fetch_buffer buffer)
{
    json_stream stream = {0};

    json_open_buffer(&stream, buffer.data, buffer.length);
    json_set_streaming(&stream, true);

    enum json_type next = json_next(&stream);

    while ((next = json_next(&stream)) == JSON_STRING) {
        size_t len;

        const char *key = json_get_string(&stream, &len);
        if (strncmp(key, "html_url", len) == 0) {
            char *url = get_string(&stream);
            puts(url);
            free(url);
        } else {
            enum json_type value_type = json_next(&stream);

            switch (value_type) {
            case JSON_ARRAY:
                json_skip_until(&stream, JSON_ARRAY_END);
                break;
            case JSON_OBJECT:
                json_skip_until(&stream, JSON_OBJECT_END);
                break;
            default:
                break;
            }
        }
    }

    if (next != JSON_OBJECT_END)
        barf("unexpected key type in json object", __func__);

    json_close(&stream);
}


const char *
get_label_(json_stream *input, const char *where)
{
    if (json_next(input) != JSON_OBJECT)
        barf("label field is not an object", where);

    const char *result = NULL;
    while (json_next(input) == JSON_STRING) {
        size_t      len = 0;
        const char *key = json_get_string(input, &len);

        if (strncmp("name", key, len) == 0) {
            if (json_next(input) != JSON_STRING)
                barf("name of the label is not a string", where);

            result = json_get_string(input, &len);
            result = sn_strndup(result, len);
        } else {
            json_next(input);
        }
    }

    return result;
}


size_t
ghcli_read_label_list(json_stream *stream, sn_sv **out)
{
    enum json_type next;
    size_t         size;

    next = json_next(stream);
    if (next != JSON_ARRAY)
        barf("expected begin of array in label list", __func__);

    size = 0;
    while ((next = json_peek(stream)) != JSON_ARRAY_END) {
        *out           = realloc(*out, sizeof(sn_sv) * (size + 1));
        const char *l  = get_label(stream);
        (*out)[size++] = SV((char *)l);
    }

    if (json_next(stream) != JSON_ARRAY_END)
        barf("expected end of array in label list", __func__);

    return size;
}

size_t
ghcli_read_user_list(json_stream *input, sn_sv **out)
{
    size_t n = 0;

    if (json_next(input) != JSON_ARRAY)
        errx(1, "Expected array for user list");

    while (json_peek(input) == JSON_OBJECT) {
        *out = realloc(*out, sizeof(**out) * (n + 1));
        (*out)[n++] = get_user_sv(input);
    }

    if (json_next(input) != JSON_ARRAY_END)
        errx(1, "Expected end of array for user list");

    return n;
}
