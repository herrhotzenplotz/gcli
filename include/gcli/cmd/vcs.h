/*
 * Copyright Nico Sonack <nsonack@herrhotzenplotz.de>
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

#ifndef GCLI_CMD_VCS_H
#define GCLI_CMD_VCS_H

#include <gcli/gcli.h>

#include <sys/queue.h>

enum {
	GCLI_CMD_VCSTYPE_UNKNOWN = 0,
	GCLI_CMD_VCSTYPE_GIT     = 1,
	GCLI_CMD_VCSTYPE_GOT,
};

struct gcli_cmd_vcs_remote {
	TAILQ_ENTRY(gcli_cmd_vcs_remote) next;

	char *name;
	char *owner;
	char *repo;
	char *host;
	gcli_forge_type forge_type;
};

struct gcli_cmd_vcs_branch {
	TAILQ_ENTRY(gcli_cmd_vcs_branch) next;

	char *name;
	char *remote;
};

struct gcli_cmd_vcs_ctx {
	TAILQ_HEAD(gcli_cmd_vcs_remotes, gcli_cmd_vcs_remote) remotes;
	TAILQ_HEAD(gcli_cmd_vcs_branches, gcli_cmd_vcs_branch) branches;
};

int gcli_cmd_vcs_get_vcstype(struct gcli_ctx *ctx);
int gcli_cmd_vcs_branchname(struct gcli_ctx *ctx, char **);
int gcli_cmd_vcs_forgetype(struct gcli_ctx *ctx, char const *remote_name);
int gcli_cmd_vcs_remote_by_forgetype(struct gcli_ctx *ctx, gcli_forge_type type,
                                     char const **out);
int gcli_cmd_vcs_repo_by_remote(struct gcli_ctx *ctx, char const *remote_name,
                                char **owner, char **repo,
                                int *forgetype);

int gcli_cmd_vcs_branch_remote(struct gcli_ctx *ctx, struct gcli_cmd_vcs_remote const **out);

int gcli_cmd_vcs_remote_head_by_owner(struct gcli_ctx *ctx, char const *owner,
                                      char const *repo, char **head_name);

int gcli_vcs_guess_forgetype_by_hostname(char const *host, gcli_forge_type *out);

#endif /* GCLI_CMD_VCS_H */
