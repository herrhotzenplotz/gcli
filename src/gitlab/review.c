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

#include <gcli/curl.h>
#include <gcli/gitlab/config.h>
#include <gcli/gitlab/review.h>
#include <gcli/json_util.h>

#include <pdjson/pdjson.h>
#include <sn/sn.h>

#include <stdlib.h>

#include <templates/gitlab/review.h>

size_t
gitlab_review_get_reviews(char const *owner,
                          char const *repo,
                          int const pr,
                          gcli_pr_review **const out)
{
    gcli_fetch_buffer  buffer   = {0};
    json_stream        stream   = {0};
    char              *url      = NULL;
    char              *next_url = NULL;
    size_t             size     = 0;

    url = sn_asprintf(
        "%s/projects/%s%%2F%s/merge_requests/%d/notes?sort=asc",
        gitlab_get_apibase(), owner, repo, pr);

    do {
        gcli_fetch(url, &next_url, &buffer);

        json_open_buffer(&stream, buffer.data, buffer.length);

        parse_gitlab_reviews(&stream, out, &size);

        json_close(&stream);
        free(url);
        free(buffer.data);
    } while ((url = next_url)); /* I hope this doesn't cause any issues */

    free(next_url);

    return size;
}
