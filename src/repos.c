/*
 * Copyright 2021, 2022 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/forges.h>
#include <gcli/github/repos.h>
#include <gcli/repos.h>
#include <gcli/cmd/table.h>

#include <stdlib.h>

int
gcli_get_repos(gcli_ctx *ctx, char const *owner, int const max,
               gcli_repo_list *const out)
{
	return gcli_forge(ctx)->get_repos(ctx, owner, max, out);
}

void
gcli_repo_free(gcli_repo *it)
{
	free(it->full_name.data);
	free(it->name.data);
	free(it->owner.data);
	free(it->date.data);
	free(it->visibility.data);
	memset(it, 0, sizeof(*it));
}

void
gcli_repos_free(gcli_repo_list *const list)
{
	for (size_t i = 0; i < list->repos_size; ++i) {
		gcli_repo_free(&list->repos[i]);
	}

	free(list->repos);

	list->repos = NULL;
	list->repos_size = 0;
}

int
gcli_repo_delete(gcli_ctx *ctx, char const *owner, char const *repo)
{
	return gcli_forge(ctx)->repo_delete(ctx, owner, repo);
}

int
gcli_repo_create(gcli_ctx *ctx, gcli_repo_create_options const *options,
                 gcli_repo *out)
{
	return gcli_forge(ctx)->repo_create(ctx, options, out);
}

int
gcli_repo_set_visibility(gcli_ctx *ctx, char const *const owner,
                         char const *const repo, gcli_repo_visibility vis)
{
	return gcli_forge(ctx)->repo_set_visibility(ctx, owner, repo, vis);
}
