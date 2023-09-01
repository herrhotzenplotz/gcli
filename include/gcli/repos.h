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

#ifndef REPOS_H
#define REPOS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sn/sn.h>
#include <gcli/gcli.h>

typedef struct gcli_repo gcli_repo;
typedef struct gcli_repo_list gcli_repo_list;
typedef struct gcli_repo_create_options gcli_repo_create_options;

struct gcli_repo {
	int   id;
	sn_sv full_name;
	sn_sv name;
	sn_sv owner;
	sn_sv date;
	sn_sv visibility;
	bool  is_fork;
};

struct gcli_repo_list {
	gcli_repo *repos;
	size_t repos_size;
};

struct gcli_repo_create_options {
	sn_sv name;
	sn_sv description;
	bool  private;
};

int gcli_get_repos(gcli_ctx *ctx, char const *owner, int max,
                   gcli_repo_list *list);

void gcli_repos_free(gcli_repo_list *list);
void gcli_repo_free(gcli_repo *it);

int gcli_repo_delete(gcli_ctx *ctx, char const *owner, char const *repo);

int gcli_repo_create(gcli_ctx *ctx, gcli_repo_create_options const *,
                     gcli_repo *out);

#endif /* REPOS_H */
