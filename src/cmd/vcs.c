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

int
gcli_cmd_vcs_forgetype(struct gcli_ctx *ctx, char const *remote_name)
{
	(void) ctx;
	(void) remote_name;

	gcli_unimplemented;

	return -1;
}

int
gcli_cmd_vcs_remote_by_forgetype(struct gcli_ctx *ctx, gcli_forge_type type,
                                 char const **out)
{
	(void) ctx;
	(void) type;
	(void) out;

	gcli_unimplemented;

	return -1;
}

int
gcli_cmd_vcs_repo_by_remote(struct gcli_ctx *ctx, char const *remote_name,
                            char const **owner, char const **repo,
                            int *forgetype)
{
	(void) ctx;
	(void) remote_name;
	(void) owner;
	(void) repo;
	(void) forgetype;

	gcli_unimplemented;

	return -1;
}
