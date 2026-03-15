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

#include <gcli/cmd/cmd.h>
#include <gcli/cmd/vcs/got.h>
#include <gcli/cmd/vcs/gotconf_parser.h>
#include <gcli/port/string.h>
#include <gcli/port/util.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* look for a file 'fname' in the .got directory */
static char *
find_file_in_dotgot(struct gcli_ctx *ctx, char const *fname)
{
	char *config_path = NULL;
	char *dotgot = NULL;
	size_t fname_len, config_path_len;

	dotgot = gcli_find_directory(".got");
	if (!dotgot) {
		gcli_warnx(g_clictx, "not a GoT worktree");
		return NULL;
	}

	/* Now search for the file in the found .got directory */
	fname_len = strlen(fname);
	config_path_len = strlen(dotgot) + 1 + fname_len + 1;

	config_path = calloc(1, config_path_len);
	snprintf(config_path, config_path_len, "%s/%s", dotgot, fname);

	if (access(config_path, F_OK) < 0) {
		gcli_warnx(ctx, "gcli: error: cannot find %s within %s", fname, dotgot);
		free(config_path);
		config_path = NULL;
	}

	free(dotgot);

	return config_path;
}

int
gcli_vcs_got_get_branchname(struct gcli_ctx *ctx, char **out)
{
	char *content = NULL, *headref_file = NULL, *brname;
	char const prefix[] = "refs/heads/";
	size_t len = 0;
	int rc = 0;

	/* resolve path to .got/head-ref and read its contents */
	headref_file = find_file_in_dotgot(ctx, "head-ref");
	if (!headref_file)
		return -1;

	rc = gcli_read_file(headref_file, &content);
	if (rc < 0)
		return rc;

	free(headref_file);
	headref_file = NULL;

	/* now check the prefix and strip it off */
	if (strncmp(prefix, content, sizeof(prefix) - 1)) {
		gcli_warnx(ctx, "gcli: vcs: got: unexpected syntax in head-ref: %s", content);

		free(content);
		content = NULL;

		return -1;
	}

	*out = brname = strdup(content + (sizeof(prefix) - 1));

	/* strip trailing newline */
	len = strlen(brname);
	if (brname[len - 1] == '\n')
		brname[len - 1] = '\0';

	free(content);
	content = NULL;

	return 0;
}

static int
find_repodir(struct gcli_ctx *ctx, char **repo_dir)
{
	char *repo_file = NULL;
	int rc = 0, len = 0;

	repo_file = find_file_in_dotgot(ctx, "repository");
	if (!repo_file)
		return -1;

	rc = gcli_read_file(repo_file, repo_dir);
	free(repo_file);
	repo_file = NULL;
	if (rc < 0)
		return rc;

	len = strlen(*repo_dir);
	if ((*repo_dir)[len - 1] == '\n')
		(*repo_dir)[--len] = '\0';

	return 0;
}

static int
find_gotconf(struct gcli_ctx *ctx, char **out)
{
	int rc = 0;
	char *repo_dir = NULL, *gotconf = NULL;

	rc = find_repodir(ctx, &repo_dir);
	if (rc < 0)
		return rc;

	gotconf = gcli_asprintf("%s/got.conf", repo_dir);
	if (access(gotconf, R_OK) < 0) {
		gcli_warnx(ctx, "gcli: vcs: got repo dir %s doesn't contain a readable got.conf", repo_dir);

		free(gotconf);
		gotconf = NULL;
		rc = -1;
	}

	free(repo_dir);
	repo_dir = NULL;

	if (out)
		*out = gotconf;

	return rc;
}

/* routine for reading in the got.conf file */
int
gcli_vcs_got_read_repoconfig(struct gcli_ctx *ctx,
                             struct gcli_cmd_vcs_ctx *vcsctx)
{
	struct gcli_gotconf_parser p = {0};
	char *gotconf, *gotconf_text;
	int rc = 0;

	TAILQ_INIT(&vcsctx->branches);
	TAILQ_INIT(&vcsctx->remotes);

	rc = find_gotconf(ctx, &gotconf);
	if (rc < 0)
		return rc;

	rc = gcli_read_file(gotconf, &gotconf_text);
	if (rc < 0)
		return rc;

	p.head = gotconf_text;

	rc = gcli_gotconf_parser_run(&p, &vcsctx->remotes);
	if (rc < 0) {
		gcli_warnx(ctx, "failed to parse %s: %s",
		           gotconf, p.error_message);
	}

	free(gotconf_text);
	gotconf_text = NULL;

	free(gotconf);
	gotconf = NULL;

	return rc;
}

int
gcli_vcs_got_get_branch_remote(struct gcli_ctx *ctx,
                               struct gcli_cmd_vcs_ctx *vcsctx,
                               char const *branch_name,
                               char **out_remote_name)
{
	struct gcli_cmd_vcs_remote *rmt = NULL;
	int rc = 0;
	char *ref_ptr, *repo_dir;

	/* GoT doesn't really have a concept of "tracking remote".
	 * Instead, scan through remotes and look for matching refs. First
	 * match succeeds. */

	rc = find_repodir(ctx, &repo_dir);
	if (rc < 0)
		return rc;

	TAILQ_FOREACH(rmt, &vcsctx->remotes, next) {
		if (rmt->forge_type == (gcli_forge_type)-1)
			continue;

		ref_ptr = gcli_asprintf("%s/refs/remotes/%s/%s",
		                        repo_dir, rmt->name, branch_name);

		if (gcli_be_verbose(ctx))
			fprintf(stderr, "gcli: vcs: got: testing for %s\n", ref_ptr);

		if (access(ref_ptr, F_OK) < 0) {
			free(ref_ptr);
			continue;
		}

		if (gcli_be_verbose(ctx))
			fprintf(stderr,
			        "gcli: vcs: got: using %s as remote for %s\n",
			        rmt->name, branch_name);

		free(ref_ptr);
		free(repo_dir);

		*out_remote_name = strdup(rmt->name);

		return 0;
	}

	free(repo_dir);
	return -1;
}

int
gcli_vcs_got_get_head_of_remote(struct gcli_ctx *ctx, char const *remote_name,
                                char **head)
{
	int rc = 0, len = 0;
	char *repo_dir = NULL, *ref_file = NULL, *ref_ptr = NULL;
	char const prefix[] = "ref: refs/remotes/";

	rc = find_repodir(ctx, &repo_dir);
	if (rc < 0)
		return rc;

	ref_file = gcli_asprintf("%s/refs/remotes/%s/HEAD", repo_dir, remote_name);

	rc = access(ref_file, R_OK);
	if (rc < 0)
		goto fail_access;

	rc = gcli_read_file(ref_file, &ref_ptr);
	if (rc < 0)
		goto fail_read;

	/* trim newline */
	if (ref_ptr[rc - 1] == '\n')
		ref_ptr[--rc] = '\0';

	if (strncmp(ref_ptr, prefix, sizeof(prefix) - 1) == 0) {
		len = strlen(remote_name);

		*head = strdup(ref_ptr + sizeof(prefix) + len);
	} else {
		rc = -1;
	}

	free(ref_ptr);

fail_read:
fail_access:
	free(ref_file);
	free(repo_dir);

	return rc;
}
