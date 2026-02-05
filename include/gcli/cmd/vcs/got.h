/* Game of Trees VCS integration
 *
 * This file is part of gcli.
 *
 * Copyright Nico Sonack <nsonack@herrhotzenplotz.de> */

#ifndef GCLI_CMD_VCS_GOT_H
#define GCLI_CMD_VCS_GOT_H

#include <gcli/gcli.h>
#include <gcli/cmd/vcs.h>

int gcli_vcs_got_get_branchname(struct gcli_ctx *ctx, char **out);

int gcli_vcs_got_read_repoconfig(struct gcli_ctx *ctx,
                                 struct gcli_cmd_vcs_ctx *out);

int gcli_vcs_got_get_branch_remote(struct gcli_ctx *ctx,
                                   struct gcli_cmd_vcs_ctx *vcsctx,
                                   char const *branch_name,
                                   char **out_remote_name);

#endif /* GCLI_CMD_VCS_GOT_H */
