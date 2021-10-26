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
#include <sn/sn.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>

int
get_int_(json_stream *input, const char *where)
{
    if (json_next(input) != JSON_NUMBER)
        errx(1, "%s: unexpected non-numeric field", where);

    return json_get_number(input);
}

const char *
get_string_(json_stream *input, const char *where)
{
    enum json_type type = json_next(input);
    if (type == JSON_NULL)
        return "<empty>";

    if (type != JSON_STRING)
        errx(1, "%s: unexpected non-string field", where);

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
        errx(1, "%s: unexpected non-boolean value", where);

    errx(42, "%s: unreachable", where);
    return false;
}

const char *
get_user_(json_stream *input, const char *where)
{
    if (json_next(input) != JSON_OBJECT)
        errx(1, "%s: user field is not an object", where);

    const char *result = NULL;
    while (json_next(input) == JSON_STRING) {
        size_t      len = 0;
        const char *key = json_get_string(input, &len);

        if (strncmp("login", key, len) == 0) {
            if (json_next(input) != JSON_STRING)
                errx(1, "%s: login of the pull request creator is not a string", where);

            result = json_get_string(input, &len);
            result = sn_strndup(result, len);
        } else {
            json_next(input);
        }
    }

    return result;
}

sn_sv
ghcli_json_escape(sn_sv it)
{
    // TODO: Improve performance of ghcli_json_escape
    sn_sv result = {0};

    for (size_t i = 0; i < it.length; ++i) {
        switch (it.data[i]) {
        case '\n': {
            result.data = realloc(result.data, result.length + 2);
            memcpy(result.data + result.length, "\\n", 2);
            result.length += 2;
        } break;
        case '"': {
            result.data = realloc(result.data, result.length + 2);
            memcpy(result.data + result.length, "\\\"", 2);
            result.length += 2;
        } break;
        case '\\': {
            result.data = realloc(result.data, result.length + 2);
            memcpy(result.data + result.length, "\\\\", 2);
            result.length += 2;
        } break;
        default: {
            result.data = realloc(result.data, result.length + 1);
            memcpy(result.data + result.length, it.data + i, 1);
            result.length += 1;
        } break;
        }
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
        errx(1, "%s: unexpected non-string field", where);

    size_t len;
    const char *it   = json_get_string(input, &len);
    char       *copy = sn_strndup(it, len);
    return SV(copy);
}
