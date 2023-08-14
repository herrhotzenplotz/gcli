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

#include <stdio.h>
#include <stdlib.h>

#include <sn/sn.h>

typedef struct gcli_comment gcli_comment;
typedef struct gcli_comment_list gcli_comment_list;
typedef struct gcli_submit_comment_opts gcli_submit_comment_opts;

struct gcli_comment {
	char const *author;         /* Login name of the comment author */
	char const *date;           /* Creation date of the comment     */
	int id;                     /* id of the comment                */
	char const *body;           /* Raw text of the comment          */
};

struct gcli_comment_list {
	gcli_comment *comments;     /* List of comments */
	size_t comments_size;       /* Size of the list */
};

struct gcli_submit_comment_opts {
	enum comment_target_type { ISSUE_COMMENT, PR_COMMENT }  target_type;
	char const *owner, *repo;
	int target_id;
	sn_sv message;
	bool always_yes;
};

void gcli_comment_list_free(gcli_comment_list *list);
void gcli_print_comment_list(gcli_comment_list const *list);
void gcli_issue_comments(char const *owner, char const *repo, int issue);
void gcli_pull_comments(char const *owner, char const *repo, int issue);
int gcli_comment_submit(gcli_submit_comment_opts opts);

#endif /* COMMENTS_H */
