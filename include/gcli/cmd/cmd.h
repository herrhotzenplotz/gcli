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

#ifndef GCLI_CMD_CMD_H
#define GCLI_CMD_CMD_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gcli/gcli.h>

#include <sn/sn.h>

extern gcli_ctx *g_clictx;

static inline char *
shift(int *argc, char ***argv)
{
	if (*argc == 0)
		errx(1, "error: Not enough arguments");

	(*argc)--;
	return *((*argv)++);
}

void version(void);
void copyright(void);
void check_owner_and_repo(const char **owner, const char **repo);

void parse_labels_options(
	int *argc, char ***argv,
	const char ***_add_labels, size_t *_add_labels_size,
	const char ***_remove_labels, size_t *_remove_labels_size);

void delete_repo(bool always_yes, const char *owner, const char *repo);

/* List of subcommand entry points */
int subcommand_api(int argc, char *argv[]);
int subcommand_ci(int argc, char *argv[]);
int subcommand_comment(int argc, char *argv[]);
int subcommand_config(int argc, char *argv[]);
int subcommand_forks(int argc, char *argv[]);
int subcommand_gists(int argc, char *argv[]);
int subcommand_issues(int argc, char *argv[]);
int subcommand_milestones(int argc, char *argv[]);
int subcommand_pipelines(int argc, char *argv[]);
int subcommand_pulls(int argc, char *argv[]);
int subcommand_repos(int argc, char *argv[]);
int subcommand_snippets(int argc, char *argv[]);
int subcommand_status(int argc, char *argv[]);

#endif /* GCLI_CMD_CMD_H */
