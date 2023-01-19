/*
 * Copyright 2022 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gcli/cmd.h>
#include <gcli/config.h>
#include <gcli/repos.h>

#include <stdlib.h>

#include <curl/curl.h>

void
copyright(void)
{
	fprintf(
		stderr,
		"Copyright 2021, 2022, 2023 Nico Sonack <nsonack@herrhotzenplotz.de>"
		" and contributors.\n");
}

void
version(void)
{
	fprintf(stderr,
	        PACKAGE_STRING"\n"
	        "Using %s\n"
	        "Using vendored pdjson library\n"
	        "Report bugs at "PACKAGE_URL".\n",
	        curl_version());
}

void
check_owner_and_repo(const char **owner, const char **repo)
{
	/* If no remote was specified, try to autodetect */
	if ((*owner == NULL) != (*repo == NULL))
		errx(1, "error: missing either explicit owner or repo");

	if (*owner == NULL)
		gcli_config_get_repo(owner, repo);
}

/* Parses (and updates) the given argument list into two seperate lists:
 *
 *   --add    -> add_labels
 *   --remove -> remove_labels
 */
void
parse_labels_options(int *argc, char ***argv,
                     const char ***_add_labels,    size_t *_add_labels_size,
                     const char ***_remove_labels, size_t *_remove_labels_size)
{
	const char **add_labels = NULL, **remove_labels = NULL;
	size_t       add_labels_size = 0, remove_labels_size = 0;

	/* Collect add/delete labels */
	while (*argc > 0) {
		if (strcmp(**argv, "add") == 0) {
			shift(argc, argv);

			add_labels = realloc(
				add_labels,
				(add_labels_size + 1) * sizeof(*add_labels));
			add_labels[add_labels_size++] = shift(argc, argv);
		} else if (strcmp(**argv, "remove") == 0) {
			shift(argc, argv);

			remove_labels = realloc(
				remove_labels,
				(remove_labels_size + 1) * sizeof(*remove_labels));
			remove_labels[remove_labels_size++] = shift(argc, argv);
		} else {
			break;
		}
	}

	*_add_labels      = add_labels;
	*_add_labels_size = add_labels_size;

	*_remove_labels      = remove_labels;
	*_remove_labels_size = remove_labels_size;
}

/* delete the repo (and ask for confirmation)
 *
 * NOTE: this procedure is here because it is used by both the forks
 * and repo subcommand. Ideally it should be moved into the 'repos'
 * code but I don't wanna make it exported from there. */
void
delete_repo(bool always_yes, const char *owner, const char *repo)
{
	bool delete = false;

	if (!always_yes) {
		delete = sn_yesno(
			"Are you sure you want to delete %s/%s?",
			owner, repo);
	} else {
		delete = true;
	}

	if (delete)
		gcli_repo_delete(owner, repo);
	else
		errx(1, "Operation aborted");
}
