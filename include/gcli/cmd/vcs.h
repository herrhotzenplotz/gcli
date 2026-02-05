/* This file is part of gcli.
 *
 * Copyright 2025 Nico Sonack <nsonack@herrhotzenplotz.de> */

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

struct gcli_cmd_vcs_ctx {
	TAILQ_HEAD(gcli_cmd_vcs_remotes, gcli_cmd_vcs_remote) remotes;
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

int gcli_vcs_guess_forgetype_by_hostname(char const *host, gcli_forge_type *out);

#endif /* GCLI_CMD_VCS_H */
