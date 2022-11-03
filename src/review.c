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

#include <gcli/color.h>
#include <gcli/config.h>
#include <gcli/curl.h>
#include <gcli/forges.h>
#include <gcli/github/config.h>
#include <gcli/json_util.h>
#include <gcli/review.h>

#include <pdjson/pdjson.h>

#include <limits.h>

void
gcli_review_print_review_table(gcli_pr_review const *const headers,
                               size_t const headers_size)
{
    for (size_t i = 0; i < headers_size; ++i) {
        if (headers[i].state) {
            printf("   %s%s%s - %s - %s%s%s\n",
                   gcli_setbold(), headers[i].author, gcli_resetbold(),
                   headers[i].date,
                   gcli_state_color_str(headers[i].state),
                   headers[i].state,
                   gcli_resetcolor());
        } else {
            printf("   %s%s%s - %s\n",
                   gcli_setbold(), headers[i].author, gcli_resetbold(),
                   headers[i].date);
        }

        pretty_print(headers[i].body, 9, 80, stdout);

        gcli_review_print_comments(
            headers[i].comments,
            headers[i].comments_size);

        putchar('\n');
    }
}

void
gcli_review_print_comments(gcli_pr_review_comment const *const comments,
                           size_t const comments_size)
{
    for (size_t i = 0; i < comments_size; ++i) {
        putchar('\n');
        printf("         PATH : %s\n"
               "         DIFF :\n",
               comments[i].path);

        pretty_print(comments[i].diff, 20, INT_MAX, stdout);
        putchar('\n');
        pretty_print(comments[i].body, 16, 80, stdout);
    }
}

void
gcli_review_reviews_free(gcli_pr_review *it, size_t const size)
{
    if (!it)
        return;

    for (size_t i = 0; i < size; ++i) {
        free(it[i].author);
        free(it[i].date);
        free(it[i].state);
        free(it[i].body);
        free(it[i].id);
    }

    gcli_review_comments_free(it->comments, it->comments_size);

    free(it);
}

void
gcli_review_comments_free(gcli_pr_review_comment *it, size_t const size)
{
    if (!it)
        return;

    for (size_t i = 0; i < size; ++i) {
        free(it[i].id);
        free(it[i].author);
        free(it[i].date);
        free(it[i].diff);
        free(it[i].path);
        free(it[i].body);
    }

    free(it);
}

size_t
gcli_review_get_reviews(char const *owner,
                        char const *repo,
                        int const pr,
                        gcli_pr_review **const out)
{
    return gcli_forge()->get_reviews(owner, repo, pr, out);
}
