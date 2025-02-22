/*
 * Copyright 2025 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/path.h>

#include <sn/sn.h>

#include <stdlib.h>
#include <string.h>

static int
split_url(struct gcli_ctx *ctx, char const *const url,
          struct gcli_path *const path)
{
	char const *repo = NULL, *owner = NULL, *id = NULL, *id_end = NULL;
	char *endptr = NULL;

	/* search for begin of owner */
	owner = strstr(url, "/repos/");
	if (owner == NULL)
		goto bad;

	owner += sizeof("/repos/") - 1;

	/* search for begin of repo */
	repo = strchr(owner, '/');
	if (repo == NULL)
		goto bad;

	/* if we found the repository, copy out the owner */
	path->as_default.owner = sn_strndup(owner, repo - owner);
	repo += 1;

	/* the type comes now, copy out the repo name first */
	id = strchr(repo, '/');
	if (id == NULL)
		goto bad;

	path->as_default.repo = sn_strndup(repo, id - repo);
	id += 1;

	id = strchr(id, '/');
	if (id == NULL)
		goto done;

	id += 1;
	id_end = strchr(id, '/');
	if (id_end == NULL)
		id_end = id + strlen(id);

	path->as_default.id = strtoul(id, &endptr, 10);
        if (endptr != id_end)
                path->as_default.id = 0;

done:
	return 0;

bad:
	return gcli_error(ctx, "bad Github URL path: %s", url);
}

/* Helpers for converting a Github forge URL to a path */
int
github_path_normalise(struct gcli_ctx *ctx, struct gcli_path const *const path,
                      struct gcli_path *const norm_path)
{
	norm_path->kind = GCLI_PATH_DEFAULT;

	switch (path->kind) {
	case GCLI_PATH_DEFAULT: {
		norm_path->as_default.owner = strdup(path->as_default.owner);
		norm_path->as_default.repo = strdup(path->as_default.repo);
		norm_path->as_default.id = path->as_default.id;
	} break;
	case GCLI_PATH_URL: {
		return split_url(ctx, path->as_url, norm_path);
	} break;
	default: {
		return gcli_error(
			ctx,
			"github_path_normalise: cannot normalise path of type %d",
			path->kind);
	}
	}

	return 0;
}
