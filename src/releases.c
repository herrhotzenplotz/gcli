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

#include <assert.h>
#include <stdlib.h>

int
gcli_get_releases(char const *owner,
                  char const *repo,
                  int const max,
                  gcli_release **const out)
{
	return gcli_forge()->get_releases(owner, repo, max, out);
}

static void
gcli_print_release(enum gcli_output_flags const flags,
                   gcli_release const *const it)
{
	if (flags & OUTPUT_LONG) {
		/* General information */
		printf("        ID : "SV_FMT"\n"
		       "      NAME : "SV_FMT"\n"
		       "    AUTHOR : "SV_FMT"\n"
		       "      DATE : "SV_FMT"\n"
		       "     DRAFT : %s\n"
		       "PRERELEASE : %s\n"
		       "    ASSETS :\n",
		       SV_ARGS(it->id),
		       SV_ARGS(it->name),
		       SV_ARGS(it->author),
		       SV_ARGS(it->date),
		       sn_bool_yesno(it->draft),
		       sn_bool_yesno(it->prerelease));

		/* asset urls */
		for (size_t i = 0; i < it->assets_size; ++i) {
			printf("           : • %s\n", it->assets[i].name);
			printf("           :   %s\n", it->assets[i].url);
		}

		/* body */
		printf("      BODY :\n");
		pretty_print(it->body.data, 13, 80, stdout);

		putchar('\n');
	} else {
		printf("%13.*s  %-24.*s  %-5.5s  %-10.10s  "SV_FMT"\n",
		       SV_ARGS(it->id),
		       SV_ARGS(it->date),
		       sn_bool_yesno(it->draft),
		       sn_bool_yesno(it->prerelease),
		       SV_ARGS(it->name));
	}
}

void
gcli_print_releases(enum gcli_output_flags const flags,
                    gcli_release const *const releases,
                    int const releases_size)
{
	if (releases_size == 0) {
		puts("No releases");
		return;
	}

	if (!(flags & OUTPUT_LONG))
		printf("%13.13s  %-24.24s  %-5.5s  %-10.10s  %s\n",
		       "ID", "DATE", "DRAFT", "PRERELEASE", "NAME");

	if (flags & OUTPUT_SORTED) {
		for (int i = releases_size; i > 0; --i)
			gcli_print_release(flags, &releases[i - 1]);
	} else {
		for (int i = 0; i < releases_size; ++i)
			gcli_print_release(flags, &releases[i]);
	}
}

void
gcli_free_releases(gcli_release *releases, int const releases_size)
{
	if (!releases)
		return;

	for (int i = 0; i < releases_size; ++i) {
		free(releases[i].name.data);
		free(releases[i].body.data);
		free(releases[i].author.data);
		free(releases[i].date.data);

		for (size_t j = 0; j < releases[i].assets_size; ++j) {
			free(releases[i].assets[j].name);
			free(releases[i].assets[j].url);
		}

		free(releases[i].assets);
	}

	free(releases);
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
