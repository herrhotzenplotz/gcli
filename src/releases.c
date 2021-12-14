/*
 * Copyright 2021 Nico Sonack <nsonack@outlook.com>
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

#include <ghcli/config.h>
#include <ghcli/github/releases.h>
#include <ghcli/releases.h>

#include <assert.h>
#include <stdlib.h>

int
ghcli_get_releases(
    const char     *owner,
    const char     *repo,
    int             max,
    ghcli_release **out)
{
    switch (ghcli_config_get_forge_type()) {
    case GHCLI_FORGE_GITHUB:
        return github_get_releases(owner, repo, max, out);
    default:
        sn_unimplemented;
    }
    return -1;
}

static void
ghcli_print_release(FILE *stream, ghcli_release *it)
{
        fprintf(stream,
                "        ID : %d\n"
                "      NAME : "SV_FMT"\n"
                "    AUTHOR : "SV_FMT"\n"
                "      DATE : "SV_FMT"\n"
                "     DRAFT : %s\n"
                "PRERELEASE : %s\n"
                "   TARBALL : "SV_FMT"\n"
                "      BODY :\n",
                it->id,
                SV_ARGS(it->name),
                SV_ARGS(it->author),
                SV_ARGS(it->date),
                sn_bool_yesno(it->draft),
                sn_bool_yesno(it->prerelease),
                SV_ARGS(it->tarball_url));

        pretty_print(it->body.data, 13, 80, stream);

        fputc('\n', stream);
}

void
ghcli_print_releases(
    FILE                    *stream,
    enum ghcli_output_order  order,
    ghcli_release           *releases,
    int                      releases_size)
{
    if (releases_size == 0) {
        fprintf(stream, "No releases\n");
        return;
    }

    if (order == OUTPUT_ORDER_SORTED) {
        for (int i = releases_size; i > 0; --i)
            ghcli_print_release(stream, &releases[i - 1]);
    } else {
        for (int i = 0; i < releases_size; ++i)
            ghcli_print_release(stream, &releases[i]);
    }
}

void
ghcli_free_releases(ghcli_release *releases, int releases_size)
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
ghcli_create_release(const ghcli_new_release *release)
{
    switch (ghcli_config_get_forge_type()) {
    case GHCLI_FORGE_GITHUB:
        github_create_release(release);
        break;
    default:
        sn_unimplemented;
        break;
    }
}

void
ghcli_release_push_asset(ghcli_new_release *release, ghcli_release_asset asset)
{
    if (release->assets_size == GHCLI_RELEASE_MAX_ASSETS)
        errx(1, "Too many assets");

    release->assets[release->assets_size++] = asset;
}

void
ghcli_delete_release(const char *owner, const char *repo, const char *id)
{
    switch (ghcli_config_get_forge_type()) {
    case GHCLI_FORGE_GITHUB:
        github_delete_release(owner, repo, id);
        break;
    default:
        sn_unimplemented;
        break;
    }
}
