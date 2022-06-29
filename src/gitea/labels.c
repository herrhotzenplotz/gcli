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

#include <ghcli/gitea/config.h>
#include <ghcli/gitea/labels.h>
#include <ghcli/github/labels.h>
#include <ghcli/json_util.h>

#include <pdjson/pdjson.h>
#include <sn/sn.h>

size_t
gitea_get_labels(
	const char   *owner,
	const char   *reponame,
	int           max,
	ghcli_label **out)
{
	return github_get_labels(owner, reponame, max, out);
}

void
gitea_create_label(
	const char  *owner,
	const char  *repo,
	ghcli_label *label)
{
	github_create_label(owner, repo, label);
}

void
gitea_delete_label(
	const char *owner,
	const char *repo,
	const char *label)
{
	char				*url		 = NULL;
	ghcli_fetch_buffer	 buffer		 = {0};
	ghcli_label			*labels		 = NULL;
	size_t				 labels_size = 0;
	int                  id			 = -1;

	/* Gitea wants the id of the label, not its name. thus fetch all
	 * the labels first to then find out what the id is we need. */
	labels_size = gitea_get_labels(owner, repo, -1, &labels);

	/* Search for the id */
	for (size_t i = 0; i < labels_size; ++i) {
		if (strcmp(labels[i].name, label) == 0) {
			id = labels[i].id;
			break;
		}
	}

	/* did we find a label? */
	if (id < 0)
		errx(1, "error: label '%s' does not exist", label);

	/* DELETE /repos/{owner}/{repo}/labels/{} */
	url = sn_asprintf("%s/repos/%s/%s/labels/%d",
					  gitea_get_apibase(),
					  owner, repo, id);

	ghcli_fetch_with_method("DELETE", url, NULL, NULL, &buffer);

	ghcli_free_labels(labels, labels_size);
	free(url);
	free(buffer.data);
}
