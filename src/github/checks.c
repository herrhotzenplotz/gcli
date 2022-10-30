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

#include <assert.h>

#include <gcli/github/checks.h>
#include <gcli/config.h>
#include <gcli/color.h>
#include <gcli/curl.h>
#include <gcli/json_util.h>

#include <templates/github/checks.h>

#include <pdjson/pdjson.h>

void
github_get_checks(char const *owner,
                  char const *repo,
                  char const *ref,
                  int const max,
                  gcli_github_checks *const out)
{
    gcli_fetch_buffer  buffer   = {0};
    char              *url      = NULL;
    char              *next_url = NULL;

    assert(out);

    url = sn_asprintf("%s/repos/%s/%s/commits/%s/check-runs",
                      gcli_get_apibase(),
                      owner, repo, ref);

    do {
        struct json_stream stream = {0};

        gcli_fetch(url, &next_url, &buffer);
        json_open_buffer(&stream, buffer.data, buffer.length);

        parse_github_checks(&stream, out);

        json_close(&stream);
        free(url);
        free(buffer.data);
    } while ((url = next_url) && ((int)(out->checks_size) < max || max < 0));
}

void
github_print_checks(gcli_github_checks const *const list)
{
    printf("%10.10s  %10.10s  %10.10s  %16.16s  %16.16s  %-s\n",
           "ID", "STATUS", "CONCLUSION", "STARTED", "COMPLETED", "NAME");

    for (size_t i = 0; i < list->checks_size; ++i) {
        printf("%10ld  %10.10s  %s%10.10s%s  %16.16s  %16.16s  %-s\n",
               list->checks[i].id,
               list->checks[i].status,
               gcli_state_color_str(list->checks[i].conclusion),
               list->checks[i].conclusion,
               gcli_resetcolor(),
               list->checks[i].started_at,
               list->checks[i].completed_at,
               list->checks[i].name);
    }
}

void
github_free_checks(gcli_github_checks *const list)
{
    for (size_t i = 0; i < list->checks_size; ++i) {
        free(list->checks[i].name);
        free(list->checks[i].status);
        free(list->checks[i].conclusion);
        free(list->checks[i].started_at);
        free(list->checks[i].completed_at);
    }

    free(list->checks);
    list->checks = NULL;
    list->checks_size = 0;
}

void
github_checks(char const *owner, char const *repo, char const *ref, int const max)
{
    gcli_github_checks checks = {0};

    github_get_checks(owner, repo, ref, max, &checks);
    github_print_checks(&checks);
    github_free_checks(&checks);
}
