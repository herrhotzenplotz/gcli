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

#ifndef GISTS_H
#define GISTS_H

#include <sn/sn.h>
#include <ghcli/ghcli.h>

typedef struct ghcli_gist      ghcli_gist;
typedef struct ghcli_gist_file ghcli_gist_file;
typedef struct ghcli_new_gist  ghcli_new_gist;

struct ghcli_gist_file {
	sn_sv  filename;
	sn_sv  language;
	sn_sv  url;
	sn_sv  type;
	size_t size;
};

struct ghcli_gist {
	sn_sv            id;
	sn_sv            owner;
	sn_sv            url;
	sn_sv            date;
	sn_sv            git_pull_url;
	sn_sv            description;
	ghcli_gist_file *files;
	size_t           files_size;
};

struct ghcli_new_gist {
	FILE       *file;
	const char *file_name;
	const char *gist_description;
};

int ghcli_get_gists(
	const char *user,
	int max,
	ghcli_gist **out);
ghcli_gist *ghcli_get_gist(
	const char *gist_id);
void ghcli_print_gists_table(
	enum ghcli_output_order  order,
	ghcli_gist              *gists,
	int                      gists_size);
void ghcli_create_gist(
	ghcli_new_gist);
void ghcli_delete_gist(
	const char *gist_id,
	bool        always_yes);

#endif /* GISTS_H */
