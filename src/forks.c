/*
 * Copyright 2021 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <ghcli/color.h>
#include <ghcli/forges.h>
#include <ghcli/forks.h>
#include <ghcli/github/forks.h>

int
ghcli_get_forks(
    const char  *owner,
    const char  *repo,
    int          max,
    ghcli_fork **out)
{
    return ghcli_forge()->get_forks(owner, repo, max, out);
}

void
ghcli_print_forks(
    enum ghcli_output_order  order,
    ghcli_fork              *forks,
    size_t                   forks_size)
{
    if (forks_size == 0) {
        puts("No forks");
        return;
    }

    printf("%-20.20s  %-20.20s  %-5.5s  %s\n",
           "OWNER", "DATE", "FORKS", "FULLNAME");

    if (order == OUTPUT_ORDER_SORTED) {
        for (size_t i = forks_size; i > 0; --i) {
            printf("%s%-20.20s%s  %-20.20s  %5d  %s\n",
                   ghcli_setbold(), forks[i - 1].owner.data, ghcli_resetbold(),
                   forks[i - 1].date.data,
                   forks[i - 1].forks,
                   forks[i - 1].full_name.data);
        }
    } else {
        for (size_t i = 0; i < forks_size; ++i) {
            printf("%s%-20.20s%s  %-20.20s  %5d  %s\n",
                   ghcli_setbold(), forks[i].owner.data, ghcli_resetbold(),
                   forks[i].date.data,
                   forks[i].forks,
                   forks[i].full_name.data);
        }
    }
}

void
ghcli_fork_create(const char *owner, const char *repo, const char *_in)
{
    ghcli_forge()->fork_create(owner, repo, _in);
}
