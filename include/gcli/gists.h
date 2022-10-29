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

#ifndef GISTS_H
#define GISTS_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gcli/gcli.h>

#include <pdjson/pdjson.h>
#include <sn/sn.h>

typedef struct gcli_gist      gcli_gist;
typedef struct gcli_gist_file gcli_gist_file;
typedef struct gcli_new_gist  gcli_new_gist;

struct gcli_gist_file {
    sn_sv  filename;
    sn_sv  language;
    sn_sv  url;
    sn_sv  type;
    size_t size;
};

struct gcli_gist {
    sn_sv           id;
    sn_sv           owner;
    sn_sv           url;
    sn_sv           date;
    sn_sv           git_pull_url;
    sn_sv           description;
    gcli_gist_file *files;
    size_t          files_size;
};

struct gcli_new_gist {
    FILE       *file;
    const char *file_name;
    const char *gist_description;
};

int gcli_get_gists(
    const char  *user,
    int          max,
    gcli_gist  **out);
gcli_gist *gcli_get_gist(
    const char *gist_id);
void gcli_print_gists_table(
    enum gcli_output_flags  flags,
    gcli_gist              *gists,
    int                     gists_size);
void gcli_create_gist(
    gcli_new_gist);
void gcli_delete_gist(
    const char *gist_id,
    bool        always_yes);

/**
 * NOTE(Nico): Because of idiots designing a web API, we get a list of
 * files in a gist NOT as an array but as an object whose keys are the
 * file names. The objects describing the files obviously contain the
 * file name again. Whatever...here's a hack. Blame GitHub.
 */
void parse_github_gist_files_idiot_hack(json_stream *stream, gcli_gist *gist);

#endif /* GISTS_H */
