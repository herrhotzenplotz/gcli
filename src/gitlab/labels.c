/*
 * Copyright 2022 Nico Sonack <nsonack@herrhotzenplotz.de>
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
#include <gcli/gitlab/labels.h>
#include <gcli/json_util.h>

#include <templates/gitlab/labels.h>

#include <pdjson/pdjson.h>

size_t
gitlab_get_labels(
    const char  *owner,
    const char  *repo,
    int          max,
    gcli_label **out)
{
    size_t              out_size = 0;
    char               *url      = NULL;
    char               *next_url = NULL;
    gcli_fetch_buffer   buffer   = {0};
    struct json_stream  stream   = {0};

    *out = NULL;

    url = sn_asprintf("%s/projects/%s%%2F%s/labels",
                      gitlab_get_apibase(), owner, repo);

    do {
        gcli_fetch(url, &next_url, &buffer);

        json_open_buffer(&stream, buffer.data, buffer.length);

        parse_gitlab_labels(&stream, out, &out_size);

        free(buffer.data);
        free(url);
        json_close(&stream);
    } while ((url = next_url) && (max == -1 || (int)out_size < max));

    return out_size;
}

void
gitlab_create_label(const char *owner, const char *repo, gcli_label *label)
{
    char               *url           = NULL;
    char               *data          = NULL;
    char               *colour_string = NULL;
    sn_sv               lname_escaped = SV_NULL;
    sn_sv               ldesc_escaped = SV_NULL;
    gcli_fetch_buffer   buffer        = {0};
    struct json_stream  stream        = {0};

    url = sn_asprintf("%s/projects/%s%%2F%s/labels",
                      gitlab_get_apibase(),
                      owner, repo);
    lname_escaped = gcli_json_escape(SV(label->name));
    ldesc_escaped = gcli_json_escape(SV(label->description));
    colour_string = sn_asprintf("%06X", (label->color>>8)&0xFFFFFF);
    data = sn_asprintf(
        "{\"name\": \""SV_FMT"\","
        "\"color\":\"#%s\","
        "\"description\":\""SV_FMT"\"}",
        SV_ARGS(lname_escaped),
        colour_string,
        SV_ARGS(ldesc_escaped));

    gcli_fetch_with_method("POST", url, data, NULL, &buffer);

    json_open_buffer(&stream, buffer.data, buffer.length);
    json_set_streaming(&stream, 1);

    parse_gitlab_label(&stream, label);

    json_close(&stream);
    free(lname_escaped.data);
    free(ldesc_escaped.data);
    free(colour_string);
    free(data);
    free(url);
    free(buffer.data);
}

void
gitlab_delete_label(
    const char *owner,
    const char *repo,
    const char *label)
{
    char              *url     = NULL;
    char              *e_label = NULL;
    gcli_fetch_buffer  buffer  = {0};

    e_label = gcli_urlencode(label);
    url     = sn_asprintf("%s/projects/%s%%2F%s/labels/%s",
                          gitlab_get_apibase(),
                          owner, repo, e_label);

    gcli_fetch_with_method("DELETE", url, NULL, NULL, &buffer);
    free(url);
    free(buffer.data);
    free(e_label);
}
