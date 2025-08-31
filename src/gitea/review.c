/*
 * Copyright 2022-2025 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/gitea/review.h>
#include <gcli/gitea/pulls.h>
#include <gcli/json_gen.h>
#include <gcli/port/err.h>
#include <gcli/port/string.h>
#include <gcli/json_util.h>

#include <templates/github/pulls.h>

int
gitea_pull_create_review(struct gcli_ctx *ctx,
                         struct gcli_pull_create_review_details const *details)
{
    char *url = NULL, *payload = NULL;
    struct gcli_jsongen gen = {0};
    char const *const state_string[] = {
        [GCLI_REVIEW_ACCEPT_CHANGES] = "APPROVED",
        [GCLI_REVIEW_REQUEST_CHANGES] = "REQUEST_CHANGES",
        [GCLI_REVIEW_COMMENT] = "COMMENT",
    };
    int rc = 0;

    rc = gitea_pull_get_review_url(ctx, &details->path, &url);
    if (rc < 0)
    	return rc;

    // For Gitea, we can directly submit the review in one step.
    // The initial POST to the reviews endpoint creates and submits the review.
    // Therefore, we don't need the separate "submit" step.
    gcli_jsongen_init(&gen);
    gcli_jsongen_begin_object(&gen);
    {
        if (details->body) {
            gcli_jsongen_objmember(&gen, "body");
            gcli_jsongen_string(&gen, details->body);
        }


        gcli_jsongen_objmember(&gen, "event");
        gcli_jsongen_string(&gen, state_string[details->review_state]);
    }
    gcli_jsongen_end_object(&gen);

    payload = gcli_jsongen_to_string(&gen);
    gcli_jsongen_free(&gen);
// Directly submit the review. Gitea's API creates and submits in one step.
rc = gcli_fetch_with_method(ctx, "POST", url, payload, NULL, NULL);

gcli_clear_ptr(&url);
gcli_clear_ptr(&payload);


    return rc;
}