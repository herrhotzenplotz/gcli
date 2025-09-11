/* This file is part of gcli.
 *
 * Copyright Nico Sonack <nsonack@herrhotzenplotz.de> */

#include <gcli/cmd/vcs.h>
#include <gcli/port/util.h>

#include <stdlib.h>

int
gcli_cmd_vcs_get_vcstype(struct gcli_ctx *ctx)
{
	char *dir = NULL;
	int rc = GCLI_CMD_VCSTYPE_UNKNOWN;

	(void) ctx; // TODO cache

	dir = gcli_find_directory(".git");
	if (dir) {
		rc = GCLI_CMD_VCSTYPE_GIT;
		goto done;
	}

	dir = gcli_find_directory(".got");
	if (dir) {
		rc = GCLI_CMD_VCSTYPE_GOT;
		goto done;
	}

done:
	free(dir);
	dir = NULL;

	return rc;
}

int
gcli_cmd_vcs_branchname(struct gcli_ctx *ctx, char **out)
{
	(void) ctx;

	if (out)
		*out = NULL;

	return -1;
}
