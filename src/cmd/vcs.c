/* This file is part of gcli.
 *
 * Copyright Nico Sonack <nsonack@herrhotzenplotz.de> */

#include <gcli/cmd/cmdconfig.h>
#include <gcli/cmd/vcs.h>
#include <gcli/cmd/vcs/git.h>
#include <gcli/cmd/vcs/got.h>
#include <gcli/port/util.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* Dispatch table for routines that call into vcs specific routines */
static struct vcs_dispatch {
	int (*get_branchname)(struct gcli_ctx *ctx,
	                      char **out);

	int (*read_repoconfig)(struct gcli_ctx *ctx,
	                       struct gcli_cmd_vcs_ctx *vcsctx);

	int (*get_branch_remote)(struct gcli_ctx *ctx,
	                         struct gcli_cmd_vcs_ctx *vcsctx,
	                         char const *branch,
	                         char **remote_name);

	int (*get_head_of_remote)(struct gcli_ctx *ctx,
	                          char const *remote_name,
	                          char **head);

} vcs_dispatches[] = {
	[GCLI_CMD_VCSTYPE_GIT] = {
		.get_branchname = gcli_vcs_git_get_current_branch,
		.read_repoconfig = gcli_vcs_git_read_repoconfig,
		.get_branch_remote = gcli_vcs_git_get_branch_remote,
	},
	[GCLI_CMD_VCSTYPE_GOT] = {
		.get_branchname = gcli_vcs_got_get_branchname,
		.read_repoconfig = gcli_vcs_got_read_repoconfig,
		.get_branch_remote = gcli_vcs_got_get_branch_remote,
	},
};

static struct gcli_cmd_vcs_ctx g_vcsctx = {0};

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

        if (gcli_be_verbose(ctx))
                fprintf(stderr, "gcli: info: vcs type is %s\n", vcs_name(rc));

	g_vcs_type = rc;

	return rc;
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

static void
ensure_config(struct gcli_ctx *ctx)
{
	static int have_read_config = 0;
	struct gcli_cmd_vcs_remote *rmt = NULL;
	int vcsty, rc;

	if (have_read_config)
		return;

	have_read_config = 1;

	TAILQ_INIT(&g_vcsctx.branches);
	TAILQ_INIT(&g_vcsctx.remotes);
	vcsty = gcli_cmd_vcs_get_vcstype(ctx);

	if (vcsty == GCLI_CMD_VCSTYPE_UNKNOWN) {
		gcli_warnx(ctx, "vcs: no or unknown vcs type");
		return;
	}

	if (!vcs_dispatches[vcsty].read_repoconfig) {
		gcli_warnx(
			ctx,
			"vcs: cannot read repo config because %s does not "
			"implement read_repoconfig",
			vcs_name(vcsty)
		);

		return;
	}

	vcs_dispatches[vcsty].read_repoconfig(ctx, &g_vcsctx);

	/* attempt a fixup of unknown forge types by scanning through the
	 * command config */
	TAILQ_FOREACH(rmt, &g_vcsctx.remotes, next) {
		if ((int)rmt->forge_type != -1)
			continue;

		if (rmt->host == NULL)
			continue; /* cannot guess if we don't have a host */

		rc = gcli_config_get_forge_type_by_host(ctx, rmt->host, &rmt->forge_type);
		if (rc < 0) {
			gcli_warnx(
				ctx,
				"vcs: failed to get forgetype of host »%s«: %s",
				rmt->host, gcli_get_error(ctx)
			);
		}
	}
}

static struct gcli_cmd_vcs_remote *
get_most_sensible_remote(struct gcli_ctx *ctx)
{
	struct gcli_cmd_vcs_remote *r;

	TAILQ_FOREACH(r, &g_vcsctx.remotes, next) {
		if (r->host == NULL || r->owner == NULL || r->repo == NULL)
			continue;

		if (r->forge_type != (gcli_forge_type)-1)
			return r;
	}

	gcli_warn(ctx, "no suitable remotes to auto-detect forge");
	return NULL;
}

int
gcli_cmd_vcs_branchname(struct gcli_ctx *ctx, char **out)
{
	VCS_CALL(get_branchname, ctx, out);
}

int
gcli_cmd_vcs_forgetype(struct gcli_ctx *ctx, char const *remote_name)
{
	struct gcli_cmd_vcs_remote *r;

	ensure_config(ctx);

	if (remote_name) {
		TAILQ_FOREACH(r, &g_vcsctx.remotes, next) {
			if (strcmp(r->name, remote_name) == 0)
				return r->forge_type;
		}
	}

	r = get_most_sensible_remote(ctx);
	if (r == NULL)
		return -1;

	return r->forge_type;
}

int
gcli_cmd_vcs_remote_by_forgetype(struct gcli_ctx *ctx, gcli_forge_type type,
                                 char const **out)
{
	struct gcli_cmd_vcs_remote *r;

	ensure_config(ctx);

	TAILQ_FOREACH(r, &g_vcsctx.remotes, next) {
		if (r->forge_type == type) {
			*out = r->name;
			return 0;
		}
	}

	gcli_warnx(ctx, "no suitable remote for forge type");
	return -1;
}

int
gcli_cmd_vcs_repo_by_remote(struct gcli_ctx *ctx, char const *remote_name,
                            char **owner, char **repo,
                            int *forgetype)
{
	struct gcli_cmd_vcs_remote *r;

	ensure_config(ctx);

	if (remote_name) {
		TAILQ_FOREACH(r, &g_vcsctx.remotes, next) {
			if (strcmp(r->name, remote_name) == 0) {
				*owner = strdup(r->owner);
				*repo  = strdup(r->repo);
				if (forgetype)
					*forgetype = r->forge_type;

				return 0;
			}
		}

		gcli_warnx(ctx, "no such remote: %s", remote_name);
		return -1;
	}

	r = get_most_sensible_remote(ctx);
	if (r == NULL)
		return -1;

	*owner = strdup(r->owner);
	*repo  = strdup(r->repo);

	if (forgetype)
		*forgetype = r->forge_type;

	return 0;
}

int
gcli_vcs_guess_forgetype_by_hostname(char const *host, gcli_forge_type *out)
{
	if (strcmp(host, "github.com") == 0)
		*out = GCLI_FORGE_GITHUB;
	else if (strcmp(host, "gitlab.com") == 0)
		*out = GCLI_FORGE_GITLAB;
	else if (strcmp(host, "codeberg.org") == 0)
		*out = GCLI_FORGE_GITEA;
	else {
		*out = -1;
		return -1;
	}

	return 0;
}

static int
vcs_remote_by_name(char const *const remote_name, struct gcli_cmd_vcs_remote const **out)
{
	struct gcli_cmd_vcs_remote *r = NULL;

	TAILQ_FOREACH(r, &g_vcsctx.remotes, next) {
		if (strcmp(remote_name, r->name) == 0) {
			*out = r;
			return 0;
		}
	}

	return -1;
}

static struct gcli_cmd_vcs_remote const *
vcs_remote_by_owner_repo(char const *owner, char const *repo)
{
	struct gcli_cmd_vcs_remote *r = NULL;

	TAILQ_FOREACH(r, &g_vcsctx.remotes, next) {
		bool const found_remote =
			r->owner && strcmp(owner, r->owner) == 0 &&
			r->repo && strcmp(repo, r->repo) == 0;

		if (found_remote)
			return r;
	}

	return NULL;
}

int
gcli_cmd_vcs_branch_remote(struct gcli_ctx *ctx, struct gcli_cmd_vcs_remote const **out)
{
	char *branch_name = NULL;
	char *remote_name = NULL;
	int vcsty = 0;
	int rc = 0;

	/* clear out for sanity */
	*out = NULL;

	ensure_config(ctx);

	rc = gcli_cmd_vcs_branchname(ctx, &branch_name);
	if (rc < 0)
		return rc;

	vcsty = gcli_cmd_vcs_get_vcstype(ctx);

	if (vcsty == GCLI_CMD_VCSTYPE_UNKNOWN) {
		gcli_warnx(ctx, "vcs: no or unknown vcs type");
		return -1;
	}

	if (!vcs_dispatches[vcsty].get_branch_remote) {
		gcli_warnx(
			ctx,
			"vcs: cannot determine tracked upstream branch because %s does not "
			"implement get_branch_remote",
			vcs_name(vcsty)
		);

		return -1;
	}

	/* query the remote name */
	rc = vcs_dispatches[vcsty].get_branch_remote(
		ctx, &g_vcsctx, branch_name, &remote_name);

	free(branch_name);

	/* resolve to actual remote */
	if (remote_name)
		rc = vcs_remote_by_name(remote_name, out);

	free(remote_name);

	return rc;
}

int
gcli_cmd_vcs_remote_head_by_owner(struct gcli_ctx *ctx, char const *owner,
                                  char const *repo, char **head_name)
{
	struct gcli_cmd_vcs_remote const *remote;

	ensure_config(ctx);

	remote = vcs_remote_by_owner_repo(owner, repo);
	if (remote == NULL)
		return -1;

	if (gcli_be_verbose(ctx))
		fprintf(stderr, "gcli: info: remote for %s/%s is %s\n",
		        owner, repo, remote->name);

	/* now ask the vcs implementation for the remote HEAD name */
	VCS_CALL(get_head_of_remote, ctx, remote->name, head_name);
}
