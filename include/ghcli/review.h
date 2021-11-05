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

#ifndef REVIEW_H
#define REVIEW_H

#include <sn/sn.h>

typedef struct ghcli_pr_review         ghcli_pr_review;
typedef struct ghcli_pr_review_comment ghcli_pr_review_comment;

struct ghcli_pr_review {
    int         id;
    const char *author;
    const char *date;
    const char *state;
    const char *body;
};

struct ghcli_pr_review_comment {
    int         id;
    const char *author;
    const char *date;
    const char *diff;
    const char *path;
    const char *body;
    int         original_position;
};

size_t ghcli_review_get_reviews(const char *org, const char *repo, int pr, ghcli_pr_review **out);
size_t ghcli_review_get_review_comments(const char *org, const char *repo, int pr, int review_id, ghcli_pr_review_comment **out);
void   ghcli_review_print_review_table(FILE *, ghcli_pr_review *, size_t);
void   ghcli_review_print_comments(FILE *out, ghcli_pr_review_comment *comments, size_t comments_size);

#endif /* REVIEW_H */
