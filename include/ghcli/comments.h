/*
 * Copyright 2021 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <stdio.h>
#include <stdlib.h>

#include <sn/sn.h>

typedef struct ghcli_comment ghcli_comment;
typedef struct ghcli_submit_comment_opts ghcli_submit_comment_opts;

struct ghcli_comment {
	const char *author;    /* Login name of the comment author */
	const char *date;      /* Creation date of the comment     */
	int         id;        /* id of the comment                */
	const char *body;      /* Raw text of the comment          */
};

struct ghcli_submit_comment_opts {
	enum comment_target_type { ISSUE_COMMENT, PR_COMMENT }  target_type;
	const char                                             *owner, *repo;
	int                                                     target_id;
	sn_sv                                                   message;
	bool                                                    always_yes;
};

void ghcli_print_comment_list(
	ghcli_comment *comments,
	size_t comments_size);
void ghcli_issue_comments(
	const char *owner,
	const char *repo,
	int issue);
void ghcli_pull_comments(
	const char *owner,
	const char *repo,
	int issue);
void ghcli_comment_submit(
	ghcli_submit_comment_opts opts);

#endif /* COMMENTS_H */
