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
    else if (value_type == JSON_FALSE)
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
