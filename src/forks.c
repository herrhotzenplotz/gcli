/*
 * Copyright 2021,2022 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/cmd/colour.h>
#include <gcli/cmd/table.h>
#include <gcli/forges.h>
#include <gcli/forks.h>
#include <gcli/github/forks.h>

int
gcli_get_forks(gcli_ctx *ctx, char const *owner, char const *repo,
               int const max, gcli_fork_list *const out)
{
	return gcli_forge(ctx)->get_forks(ctx, owner, repo, max, out);
}

int
gcli_fork_create(gcli_ctx *ctx, char const *owner, char const *repo,
                 char const *_in)
{
	return gcli_forge(ctx)->fork_create(ctx, owner, repo, _in);
}

void
gcli_fork_free(gcli_fork *fork)
{
	free(fork->full_name.data);
	free(fork->owner.data);
	free(fork->date.data);
}

void
gcli_forks_free(gcli_fork_list *const list)
{
	for (size_t i = 0; i < list->forks_size; ++i) {
		gcli_fork_free(&list->forks[i]);
	}

	free(list->forks);

	list->forks = NULL;
	list->forks_size = 0;
}
