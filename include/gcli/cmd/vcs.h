/* This file is part of gcli.
 *
 * Copyright 2025 Nico Sonack <nsonack@herrhotzenplotz.de> */

#ifndef GCLI_CMD_VCS_H
#define GCLI_CMD_VCS_H

#include <gcli/gcli.h>

enum {
	GCLI_CMD_VCSTYPE_UNKNOWN = 0,
	GCLI_CMD_VCSTYPE_GIT     = 1,
	GCLI_CMD_VCSTYPE_GOT,
};

int gcli_cmd_vcs_get_vcstype(struct gcli_ctx *ctx);
int gcli_cmd_vcs_branchname(struct gcli_ctx *ctx, char **);
int gcli_cmd_vcs_forgetype(struct gcli_ctx *ctx, char const *remote_name);
int gcli_cmd_vcs_remote_by_forgetype(struct gcli_ctx *ctx, gcli_forge_type type,
                                     char const **out);
int gcli_cmd_vcs_repo_by_remote(struct gcli_ctx *ctx, char const *remote_name,
                                char const **owner, char const **repo,
                                int *forgetype);


#endif /* GCLI_CMD_VCS_H */
