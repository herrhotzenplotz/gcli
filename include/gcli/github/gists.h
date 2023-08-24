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

#ifndef GCLI_GITHUB_GISTS_H
#define GCLI_GITHUB_GISTS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gcli/gcli.h>

#include <pdjson/pdjson.h>
#include <sn/sn.h>

typedef struct gcli_gist      gcli_gist;
typedef struct gcli_gist_list gcli_gist_list;
typedef struct gcli_gist_file gcli_gist_file;
typedef struct gcli_new_gist  gcli_new_gist;

struct gcli_gist_file {
	sn_sv  filename;
	sn_sv  language;
	sn_sv  url;
	sn_sv  type;
	size_t size;
};

struct gcli_gist_list {
	gcli_gist *gists;
	size_t gists_size;
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
	char const *file_name;
	char const *gist_description;
};

int gcli_get_gists(gcli_ctx *ctx, char const *user, int max,
                   gcli_gist_list *list);

int gcli_get_gist(gcli_ctx *ctx, char const *gist_id, gcli_gist *out);

int gcli_create_gist(gcli_ctx *ctx, gcli_new_gist);

int gcli_delete_gist(gcli_ctx *ctx, char const *gist_id);

void gcli_gists_free(gcli_gist_list *list);
void gcli_gist_free(gcli_gist *g);

/**
 * NOTE(Nico): Because of idiots designing a web API, we get a list of
 * files in a gist NOT as an array but as an object whose keys are the
 * file names. The objects describing the files obviously contain the
 * file name again. Whatever...here's a hack. Blame GitHub.
 */
void parse_github_gist_files_idiot_hack(gcli_ctx *ctx, json_stream *stream,
                                        gcli_gist *gist);

#endif /* GCLI_GITHUB_GISTS_H */
