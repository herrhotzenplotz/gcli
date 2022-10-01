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

#include <stdlib.h>

int
gcli_get_repos(const char *owner, int max, gcli_repo **out)
{
    return gcli_forge()->get_repos(owner, max, out);
}


static void
gcli_print_repo(gcli_repo *repo)
{
    printf("%-4.4s  %-10.10s  %-16.16s  %-s\n",
           sn_bool_yesno(repo->is_fork),
           repo->visibility.data,
           repo->date.data,
           repo->full_name.data);
}

void
gcli_print_repos_table(
    enum gcli_output_order  order,
    gcli_repo              *repos,
    size_t                  repos_size)
{
    if (repos_size == 0) {
        puts("No repos");
        return;
    }

    printf("%-4.4s  %-10.10s  %-16.16s  %-s\n",
           "FORK", "VISBLTY", "DATE", "FULLNAME");

    if (order == OUTPUT_ORDER_SORTED) {
        for (size_t i = repos_size; i > 0; --i)
            gcli_print_repo(&repos[i - 1]);
    } else {
        for (size_t i = 0; i < repos_size; ++i)
            gcli_print_repo(&repos[i]);
    }
}

void
gcli_repos_free(gcli_repo *repos, size_t repos_size)
{
    for (size_t i = 0; i < repos_size; ++i) {
        free(repos[i].full_name.data);
        free(repos[i].name.data);
        free(repos[i].owner.data);
        free(repos[i].date.data);
        free(repos[i].visibility.data);
    }

    free(repos);
}

int
gcli_get_own_repos(int max, gcli_repo **out)
{
    return gcli_forge()->get_own_repos(max, out);
}

void
gcli_repo_delete(const char *owner, const char *repo)
{
    gcli_forge()->repo_delete(owner, repo);
}

gcli_repo *
gcli_repo_create(
    const gcli_repo_create_options *options) /* Options descriptor */
{
    return gcli_forge()->repo_create(options);
}
