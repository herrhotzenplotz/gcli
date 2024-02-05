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

#ifndef COMMENTS_H
#define COMMENTS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gcli/gcli.h>

#include <stdio.h>
#include <stdlib.h>

#include <sn/sn.h>

struct gcli_comment {
	char *author;               /* Login name of the comment author */
	char *date;                 /* Creation date of the comment     */
	gcli_id id;                 /* id of the comment                */
	char *body;                 /* Raw text of the comment          */
};

struct gcli_comment_list {
	struct gcli_comment *comments;     /* List of comments */
	size_t comments_size;              /* Size of the list */
};

struct gcli_submit_comment_opts {
	enum comment_target_type { ISSUE_COMMENT, PR_COMMENT }  target_type;
	char const *owner, *repo;
	gcli_id target_id;
	char const *message;
};

void gcli_comments_free(struct gcli_comment_list *list);

void gcli_comment_free(struct gcli_comment *const it);

int gcli_get_issue_comments(struct gcli_ctx *ctx, char const *owner,
                            char const *repo, gcli_id issue,
                            struct gcli_comment_list *out);

int gcli_get_pull_comments(struct gcli_ctx *ctx, char const *owner,
                           char const *repo, gcli_id issue,
                           struct gcli_comment_list *out);

int gcli_comment_submit(struct gcli_ctx *ctx, struct gcli_submit_comment_opts opts);

#endif /* COMMENTS_H */
