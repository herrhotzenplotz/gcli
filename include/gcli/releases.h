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

#ifndef RELEASES_H
#define RELEASES_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sn/sn.h>
#include <gcli/gcli.h>

typedef struct gcli_release       gcli_release;
typedef struct gcli_new_release   gcli_new_release;
typedef struct gcli_release_asset gcli_release_asset;

struct gcli_release {
    sn_sv   id;                 /* Probably shouldn't be called id */
    sn_sv  *asset_urls;
    size_t  asset_urls_size;
    sn_sv   name;
    sn_sv   body;
    sn_sv   author;
    sn_sv   date;
    sn_sv   upload_url;
    sn_sv   html_url;
    bool    draft;
    bool    prerelease;
};

struct gcli_release_asset {
    char *label;
    char *name;
    char *path;
};

#define GCLI_RELEASE_MAX_ASSETS 16
struct gcli_new_release {
    char const         *owner;
    char const         *repo;
    char const         *tag;
    char const         *name;
    sn_sv               body;
    char const         *commitish;
    bool                draft;
    bool                prerelease;
    gcli_release_asset  assets[GCLI_RELEASE_MAX_ASSETS];
    size_t              assets_size;
};

int gcli_get_releases(char const           *owner,
                      char const           *repo,
                      int const             max,
                      gcli_release **const  out);
void gcli_print_releases(enum gcli_output_flags const flags,
                         gcli_release const *const    releases,
                         int const                    releases_size);
void gcli_free_releases(gcli_release *, int const);
void gcli_create_release(gcli_new_release const *);
void gcli_release_push_asset(gcli_new_release *const, gcli_release_asset);
void gcli_delete_release(char const *const owner,
                         char const *const repo,
                         char const *const id);

#endif /* RELEASES_H */
