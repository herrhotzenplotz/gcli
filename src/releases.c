/*
 * Copyright 2021,2022 Nico Sonack <nsonack@herrhotzenplotz.de>
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
#include <gcli/table.h>

#include <assert.h>
#include <stdlib.h>

int
gcli_get_releases(char const *owner,
                  char const *repo,
                  int const max,
                  gcli_release_list *const list)
{
	return gcli_forge()->get_releases(owner, repo, max, list);
}

static void
gcli_print_release(enum gcli_output_flags const flags,
                   gcli_release const *const it)
{
	gcli_dict dict;

	(void) flags;

	dict = gcli_dict_begin();

	gcli_dict_add(dict,        "ID",         0, 0, SV_FMT, SV_ARGS(it->id));
	gcli_dict_add(dict,        "NAME",       0, 0, SV_FMT, SV_ARGS(it->name));
	gcli_dict_add(dict,        "AUTHOR",     0, 0, SV_FMT, SV_ARGS(it->author));
	gcli_dict_add(dict,        "DATE",       0, 0, SV_FMT, SV_ARGS(it->date));
	gcli_dict_add_string(dict, "DRAFT",      0, 0, sn_bool_yesno(it->draft));
	gcli_dict_add_string(dict, "PRERELEASE", 0, 0, sn_bool_yesno(it->prerelease));
	gcli_dict_add_string(dict, "ASSETS",     0, 0, "");

	/* asset urls */
	for (size_t i = 0; i < it->assets_size; ++i) {
		gcli_dict_add(dict, "", 0, 0, "• %s", it->assets[i].name);
		gcli_dict_add(dict, "", 0, 0, "  %s", it->assets[i].url);
	}

	gcli_dict_end(dict);

	/* body */
	if (it->body.length) {
		putchar('\n');
		pretty_print(it->body.data, 13, 80, stdout);
	}

	putchar('\n');
}

static void
gcli_print_releases_long(enum gcli_output_flags const flags,
                         gcli_release_list const *const list,
                         int const max)
{
	int n;

	/* Determine how many items to print */
	if (max < 0 || (size_t)(max) > list->releases_size)
		n = list->releases_size;
	else
		n = max;

	if (flags & OUTPUT_SORTED) {
		for (int i = 0; i < n; ++i)
			gcli_print_release(flags, &list->releases[n-i-1]);
	} else {
		for (int i = 0; i < n; ++i)
			gcli_print_release(flags, &list->releases[i]);
	}
}

static void
gcli_print_releases_short(enum gcli_output_flags const flags,
                          gcli_release_list const *const list,
                          int const max)
{
	size_t n;
	gcli_tbl table;
	gcli_tblcoldef cols[] = {
		{ .name = "ID",         .type = GCLI_TBLCOLTYPE_SV,   .flags = 0 },
		{ .name = "DATE",       .type = GCLI_TBLCOLTYPE_SV,   .flags = 0 },
		{ .name = "DRAFT",      .type = GCLI_TBLCOLTYPE_BOOL, .flags = 0 },
		{ .name = "PRERELEASE", .type = GCLI_TBLCOLTYPE_BOOL, .flags = 0 },
		{ .name = "NAME",       .type = GCLI_TBLCOLTYPE_SV,   .flags = 0 },
	};

	if (max < 0 || (size_t)(max) > list->releases_size)
		n = list->releases_size;
	else
		n = max;

	table = gcli_tbl_begin(cols, ARRAY_SIZE(cols));
	if (!table)
		errx(1, "error: could not init table");

	if (flags & OUTPUT_SORTED) {
		for (size_t i = 0; i < n; ++i) {
			gcli_tbl_add_row(table,
			                 list->releases[n-i-1].id,
			                 list->releases[n-i-1].date,
			                 list->releases[n-i-1].draft,
			                 list->releases[n-i-1].prerelease,
			                 list->releases[n-i-1].name);
		}
	} else {
		for (size_t i = 0; i < n; ++i) {
			gcli_tbl_add_row(table,
			                 list->releases[i].id,
			                 list->releases[i].date,
			                 list->releases[i].draft,
			                 list->releases[i].prerelease,
			                 list->releases[i].name);
		}
	}

	gcli_tbl_end(table);
}

void
gcli_print_releases(enum gcli_output_flags const flags,
                    gcli_release_list const *const list,
                    int const max)
{
	if (list->releases_size == 0) {
		puts("No releases");
		return;
	}

	if (flags & OUTPUT_LONG)
		gcli_print_releases_long(flags, list, max);
	else
		gcli_print_releases_short(flags, list, max);
}

void
gcli_free_releases(gcli_release_list *const list)
{
	for (size_t i = 0; i < list->releases_size; ++i) {
		free(list->releases[i].name.data);
		free(list->releases[i].body.data);
		free(list->releases[i].author.data);
		free(list->releases[i].date.data);

		for (size_t j = 0; j < list->releases[i].assets_size; ++j) {
			free(list->releases[i].assets[j].name);
			free(list->releases[i].assets[j].url);
		}

		free(list->releases[i].assets);
	}

	free(list->releases);

	list->releases = NULL;
	list->releases_size = 0;
}

void
gcli_create_release(gcli_new_release const *release)
{
	gcli_forge()->create_release(release);
}

void
gcli_release_push_asset(gcli_new_release *const release,
                        gcli_release_asset_upload const asset)
{
	if (release->assets_size == GCLI_RELEASE_MAX_ASSETS)
		errx(1, "Too many assets");

	release->assets[release->assets_size++] = asset;
}

void
gcli_delete_release(char const *const owner,
                    char const *const repo,
                    char const *const id)
{
	gcli_forge()->delete_release(owner, repo, id);
}
