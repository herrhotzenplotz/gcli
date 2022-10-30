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

#ifndef REVIEW_H
#define REVIEW_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sn/sn.h>

typedef struct gcli_pr_review         gcli_pr_review;
typedef struct gcli_pr_review_comment gcli_pr_review_comment;

struct gcli_pr_review_comment {
    char *id;
    char *author;
    char *date;
    char *diff;
    char *path;
    char *body;
    int   original_position;
};

struct gcli_pr_review {
    char                   *id;
    char                   *author;
    char                   *date;
    char                   *state;
    char                   *body;
    gcli_pr_review_comment *comments;
    size_t                  comments_size;
};

void gcli_review_reviews_free(
    gcli_pr_review *it,
    size_t const    size);

void gcli_review_comments_free(
    gcli_pr_review_comment *it,
    size_t const            size);

size_t gcli_review_get_reviews(
    char const             *owner,
    char const             *repo,
    int const               pr,
    gcli_pr_review **const  out);

void gcli_review_print_review_table(
    gcli_pr_review const *const headers,
    size_t const headers_size);

void gcli_review_print_comments(
    gcli_pr_review_comment const *const comments,
    size_t const comments_size);

#endif /* REVIEW_H */
