/* Game of Trees VCS integration
 *
 * This file is part of gcli.
 *
 * Copyright Nico Sonack <nsonack@herrhotzenplotz.de> */

#ifndef GCLI_CMD_VCS_GOT_H
#define GCLI_CMD_VCS_GOT_H

#include <gcli/gcli.h>

int gcli_vcs_got_get_branchname(struct gcli_ctx *ctx, char **out);

#endif /* GCLI_CMD_VCS_GOT_H */
