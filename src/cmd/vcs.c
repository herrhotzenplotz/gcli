/* This file is part of gcli.
 *
 * Copyright Nico Sonack <nsonack@herrhotzenplotz.de> */

#include <gcli/cmd/vcs.h>
#include <gcli/port/util.h>
#include <gcli/cmd/vcs/git.h>
#include <gcli/cmd/vcs/got.h>

#include <assert.h>
#include <stdlib.h>

/* Dispatch table for routines that call into vcs specific routines */
static struct vcs_dispatch {
	int (*get_branchname)(struct gcli_ctx *ctx,
	                      char **out);

	int (*get_forgetype)(struct gcli_ctx *ctx,
	                     char const *const remote_name);

	int (*get_remote_by_forgetype)(struct gcli_ctx *ctx,
	                               gcli_forge_type type,
	                               char const **out);

	int (*get_repo_by_remote)(struct gcli_ctx *ctx,
	                          char const *remote_name,
	                          char const **owner,
	                          char const **repo,
	                          int *forgetype);
} vcs_dispatches[] = {
	[GCLI_CMD_VCSTYPE_GIT] = {
		.get_branchname = gcli_vcs_git_get_current_branch,
		.get_forgetype = gcli_vcs_git_get_forgetype,
		.get_remote_by_forgetype = gcli_vcs_git_get_remote,
		.get_repo_by_remote = gcli_vcs_git_repo_by_remote,
	},
	[GCLI_CMD_VCSTYPE_GOT] = {
		.get_branchname = gcli_vcs_got_get_branchname,
	},
};

int
gcli_cmd_vcs_get_vcstype(struct gcli_ctx *ctx)
{
	static int g_vcs_type = GCLI_CMD_VCSTYPE_UNKNOWN;
	char *dir = NULL;
	int rc = GCLI_CMD_VCSTYPE_UNKNOWN;

	(void) ctx;

	if (g_vcs_type)
		return g_vcs_type;

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

	g_vcs_type = rc;

	return rc;
}

static char const *
vcs_name(int type)
{
	switch (type) {
	case GCLI_CMD_VCSTYPE_UNKNOWN: return "unknown";
	case GCLI_CMD_VCSTYPE_GOT:     return "got";
	case GCLI_CMD_VCSTYPE_GIT:     return "git";
	}

	assert(0 && "unreachable");
}

#define VCS_CALL(dispatcher, ctx, ...)                                                             \
do {                                                                                               \
	int vcsty = gcli_cmd_vcs_get_vcstype(ctx);                                                 \
                                                                                                   \
	if (vcsty == GCLI_CMD_VCSTYPE_UNKNOWN)                                                     \
		return gcli_warnx(ctx, "vcs: %s failed: no or unknown vcs type", #dispatcher), -1; \
                                                                                                   \
	if (!vcs_dispatches[vcsty].dispatcher)                                                     \
		return gcli_warnx(ctx, "vcs: %s failed: not implement for %s",                     \
		                  #dispatcher, vcs_name(vcsty)), -1;                               \
                                                                                                   \
	return vcs_dispatches[vcsty].dispatcher(ctx, __VA_ARGS__);                                 \
} while (0)

int
gcli_cmd_vcs_branchname(struct gcli_ctx *ctx, char **out)
{
	VCS_CALL(get_branchname, ctx, out);
}

int
gcli_cmd_vcs_forgetype(struct gcli_ctx *ctx, char const *remote_name)
{
	VCS_CALL(get_forgetype, ctx, remote_name);
}

int
gcli_cmd_vcs_remote_by_forgetype(struct gcli_ctx *ctx, gcli_forge_type type,
                                 char const **out)
{
	VCS_CALL(get_remote_by_forgetype, ctx, type, out);
}

int
gcli_cmd_vcs_repo_by_remote(struct gcli_ctx *ctx, char const *remote_name,
                            char const **owner, char const **repo,
                            int *forgetype)
{
	VCS_CALL(get_repo_by_remote, ctx, remote_name, owner, repo, forgetype);
}
