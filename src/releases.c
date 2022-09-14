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

#include <gcli/forges.h>
#include <gcli/github/releases.h>
#include <gcli/releases.h>

#include <assert.h>
#include <stdlib.h>

int
gcli_get_releases(
    const char    *owner,
    const char    *repo,
    int            max,
    gcli_release **out)
{
    return gcli_forge()->get_releases(owner, repo, max, out);
}

static void
gcli_print_release(gcli_release *it)
{
    printf("        ID : "SV_FMT"\n"
           "      NAME : "SV_FMT"\n"
           "    AUTHOR : "SV_FMT"\n"
           "      DATE : "SV_FMT"\n"
           "     DRAFT : %s\n"
           "PRERELEASE : %s\n"
           "   TARBALL : "SV_FMT"\n"
           "      BODY :\n",
           SV_ARGS(it->id),
           SV_ARGS(it->name),
           SV_ARGS(it->author),
           SV_ARGS(it->date),
           sn_bool_yesno(it->draft),
           sn_bool_yesno(it->prerelease),
           SV_ARGS(it->tarball_url));

    pretty_print(it->body.data, 13, 80, stdout);

    putchar('\n');
}

void
gcli_print_releases(
    enum gcli_output_order  order,
    gcli_release           *releases,
    int                     releases_size)
{
    if (releases_size == 0) {
        puts("No releases");
        return;
    }

    if (order == OUTPUT_ORDER_SORTED) {
        for (int i = releases_size; i > 0; --i)
            gcli_print_release(&releases[i - 1]);
    } else {
        for (int i = 0; i < releases_size; ++i)
            gcli_print_release(&releases[i]);
    }
}

void
gcli_free_releases(gcli_release *releases, int releases_size)
{
    if (!releases)
        return;

    for (int i = 0; i < releases_size; ++i) {
        free(releases[i].tarball_url.data);
        free(releases[i].name.data);
        free(releases[i].body.data);
        free(releases[i].author.data);
        free(releases[i].date.data);
    }

    free(releases);
}

void
gcli_create_release(const gcli_new_release *release)
{
    gcli_forge()->create_release(release);
}

void
gcli_release_push_asset(gcli_new_release *release, gcli_release_asset asset)
{
    if (release->assets_size == GCLI_RELEASE_MAX_ASSETS)
        errx(1, "Too many assets");

    release->assets[release->assets_size++] = asset;
}

void
gcli_delete_release(const char *owner, const char *repo, const char *id)
{
    gcli_forge()->delete_release(owner, repo, id);
}
