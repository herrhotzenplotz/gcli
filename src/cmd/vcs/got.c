/* Game of Trees VCS integration
 *
 * This file is part of gcli.
 *
 * Copyright Nico Sonack <nsonack@herrhotzenplotz.de> */

#include <gcli/cmd/cmd.h>
#include <gcli/cmd/vcs/got.h>
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
