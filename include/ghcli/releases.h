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

#ifndef RELEASES_H
#define RELEASES_H

#include <sn/sn.h>

typedef struct ghcli_release       ghcli_release;
typedef struct ghcli_new_release   ghcli_new_release;
typedef struct ghcli_release_asset ghcli_release_asset;

struct ghcli_release {
    int   id;
    sn_sv tarball_url;
    sn_sv name;
    sn_sv body;
    sn_sv author;
    sn_sv date;
    sn_sv upload_url;
    sn_sv html_url;
    bool  draft;
    bool  prerelease;
};

struct ghcli_release_asset {
    char *label;
    char *name;
    char *path;
};

#define GHCLI_RELEASE_MAX_ASSETS 16
struct ghcli_new_release {
    const char          *owner;
    const char          *repo;
    const char          *tag;
    const char          *name;
    sn_sv                body;
    const char          *commitish;
    bool                 draft;
    bool                 prerelease;
    ghcli_release_asset  assets[GHCLI_RELEASE_MAX_ASSETS];
    size_t               assets_size;
};

int ghcli_get_releases(
    const char     *owner,
    const char     *repo,
    int             max,
    ghcli_release **out);
void ghcli_print_releases(
    FILE *,
    ghcli_release *,
    int);
void ghcli_free_releases(
    ghcli_release *,
    int);
void ghcli_create_release(
    const ghcli_new_release *);
void ghcli_release_push_asset(
    ghcli_new_release *,
    ghcli_release_asset);
void ghcli_delete_release(
    const char *owner,
    const char *repo,
    const char *id);

#endif /* RELEASES_H */
