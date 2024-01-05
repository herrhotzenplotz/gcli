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

#ifndef GITLAB_REPOS_H
#define GITLAB_REPOS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gcli/repos.h>

int gitlab_get_repo(struct gcli_ctx *ctx, char const *owner, char const *repo,
                    gcli_repo *out);

int gitlab_get_repos(struct gcli_ctx *ctx, char const *owner, int max,
                     gcli_repo_list *out);

int gitlab_get_own_repos(struct gcli_ctx *ctx, int max, gcli_repo_list *out);

int gitlab_repo_delete(struct gcli_ctx *ctx, char const *owner, char const *repo);

int gitlab_repo_create(struct gcli_ctx *ctx,
                       gcli_repo_create_options const *options, gcli_repo *out);

int gitlab_repo_set_visibility(struct gcli_ctx *ctx, char const *const owner,
                               char const *const repo,
                               gcli_repo_visibility vis);

#endif /* GITLAB_REPOS_H */
