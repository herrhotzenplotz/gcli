/*
 * Copyright 2023 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#ifndef GCLI_MILESTONES_H
#define GCLI_MILESTONES_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>

#include <gcli/issues.h>
#include <gcli/pulls.h>

typedef struct gcli_milestone gcli_milestone;
typedef struct gcli_milestone_list gcli_milestone_list;

struct gcli_milestone {
	int id;
	char *title;
	char *state;
	char *created_at;

	/* Extended info */
	char *description;
	char *updated_at;
	char *due_date;
	int expired;

	/* Github and Gitea Specific */
	int open_issues;
	int closed_issues;
};

struct gcli_milestone_list {
	gcli_milestone *milestones;
	size_t milestones_size;
};

struct gcli_milestone_create_args {
	/* These are const because they are coming from either an mmapped
	 * buffer or from command line arguments. They should never ever
	 * get free()-ed */
	char const *title;
	char const *description;
	char const *owner;
	char const *repo;
};

int gcli_get_milestones(char const *owner,
                        char const *repo,
                        int max,
                        gcli_milestone_list *out);

int gcli_get_milestone(char const *owner,
                       char const *repo,
                       int max,
                       gcli_milestone *out);

int gcli_create_milestone(struct gcli_milestone_create_args const *args);

int gcli_delete_milestone(char const *owner,
                          char const *repo,
                          int milestone);

void gcli_print_milestones(gcli_milestone_list const *it,
                           int max);

void gcli_print_milestone(gcli_milestone const *it);

void gcli_free_milestone(gcli_milestone *it);
void gcli_free_milestones(gcli_milestone_list *it);

int gcli_milestone_get_issues(char const *owner,
                              char const *repo,
                              int milestone,
                              gcli_issue_list *out);

int gcli_milestone_set_duedate(char const *owner,
                               char const *repo,
                               int milestone,
                               char const *date);

#endif /* GCLI_MILESTONES_H */
