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

#include <gcli/curl.h>
#include <gcli/gitea/config.h>
#include <gcli/gitea/issues.h>
#include <gcli/gitea/labels.h>
#include <gcli/github/issues.h>
#include <gcli/json_util.h>
#include <gcli/labels.h>

#include <pdjson/pdjson.h>

int
gitea_get_issues(char const *owner,
                 char const *repo,
                 bool const all,
                 int const max,
                 gcli_issue **const out)
{
	return github_get_issues(owner, repo, all, max, out);
}

void
gitea_get_issue_summary(char const *owner,
                        char const *repo,
                        int const issue_number,
                        gcli_issue *const out)
{
	github_get_issue_summary(owner, repo, issue_number, out);
}

void
gitea_submit_issue(gcli_submit_issue_options opts,
                   gcli_fetch_buffer *const out)
{
	github_perform_submit_issue(opts, out);
}

/* Gitea has closed, Github has close ... go figure */
static void
gitea_issue_patch_state(char const *owner,
                        char const *repo,
                        int const issue_number,
                        char const *const state)
{
	gcli_fetch_buffer  json_buffer = {0};
	char              *url         = NULL;
	char              *data        = NULL;
	char              *e_owner     = NULL;
	char              *e_repo      = NULL;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf(
		"%s/repos/%s/%s/issues/%d",
		gitea_get_apibase(),
		e_owner, e_repo,
		issue_number);
	data = sn_asprintf("{ \"state\": \"%s\"}", state);

	gcli_fetch_with_method("PATCH", url, data, NULL, &json_buffer);

	free(data);
	free(url);
	free(e_owner);
	free(e_repo);
	free(json_buffer.data);
}

void
gitea_issue_close(char const *owner,
                  char const *repo,
                  int const issue_number)
{
	gitea_issue_patch_state(owner, repo, issue_number, "closed");
}

void
gitea_issue_reopen(char const *owner,
                   char const *repo,
                   int const issue_number)
{
	gitea_issue_patch_state(owner, repo, issue_number, "open");
}

void
gitea_issue_assign(char const *owner,
                   char const *repo,
                   int const issue_number,
                   char const *const assignee)
{
	gcli_fetch_buffer  buffer           = {0};
	sn_sv              escaped_assignee = SV_NULL;
	char              *post_fields      = NULL;
	char              *url              = NULL;
	char              *e_owner          = NULL;
	char              *e_repo           = NULL;

	escaped_assignee = gcli_json_escape(SV((char *)assignee));
	post_fields = sn_asprintf("{ \"assignees\": [\""SV_FMT"\"] }",
	                          SV_ARGS(escaped_assignee));

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf(
		"%s/repos/%s/%s/issues/%d",
		gitea_get_apibase(), e_owner, e_repo, issue_number);

	gcli_fetch_with_method("PATCH", url, post_fields, NULL, &buffer);

	free(buffer.data);
	free(escaped_assignee.data);
	free(post_fields);
	free(e_owner);
	free(e_repo);
	free(url);
}

/* Return the stringified id of the given label */
static char *
get_id_of_label(char const *label_name,
                gcli_label const *const labels,
                size_t const labels_size)
{
	for (size_t i = 0; i < labels_size; ++i)
		if (strcmp(labels[i].name, label_name) == 0)
			return sn_asprintf("%ld", labels[i].id);
	return NULL;
}

static char **
label_names_to_ids(char const *owner,
                   char const *repo,
                   char const *const names[],
                   size_t const names_size)
{
	gcli_label  *labels      = NULL;
	size_t       labels_size = 0;
	char       **ids         = NULL;
	size_t       ids_size    = 0;

	labels_size = gitea_get_labels(owner, repo, -1, &labels);

	for (size_t i = 0; i < names_size; ++i) {
		char *const label_id = get_id_of_label(
			names[i], labels, labels_size);

		if (!label_id)
			errx(1, "error: no such label '%s'", names[i]);

		ids = realloc(ids, sizeof(*ids) * (ids_size +1));
		ids[ids_size++] = label_id;
	}

	gcli_free_labels(labels, labels_size);

	return ids;
}

static void
free_id_list(char *list[], size_t const list_size)
{
	for (size_t i = 0; i < list_size; ++i) {
		free(list[i]);
	}
	free(list);
}

void
gitea_issue_add_labels(char const *owner,
                       char const *repo,
                       int const issue,
                       char const *const labels[],
                       size_t const labels_size)
{
	char              *list   = NULL;
	char              *data   = NULL;
	char              *url    = NULL;
	gcli_fetch_buffer  buffer = {0};

	/* First, convert to ids */
	char **ids = label_names_to_ids(owner, repo, labels, labels_size);

	/* Construct json payload */

	/* Note: http://www.c-faq.com/ansi/constmismatch.html */
	list = sn_join_with((char const **)ids, labels_size, ",");
	data = sn_asprintf("{ \"labels\": [%s] }", list);

	url = sn_asprintf("%s/repos/%s/%s/issues/%d/labels",
	                  gitea_get_apibase(), owner, repo, issue);
	gcli_fetch_with_method("POST", url, data, NULL, &buffer);

	free(list);
	free(data);
	free(url);
	free(buffer.data);
	free_id_list(ids, labels_size);
}

void
gitea_issue_remove_labels(char const *owner,
                          char const *repo,
                          int const issue,
                          char const *const labels[],
                          size_t const labels_size)
{
	/* Unfortunately the gitea api does not give us an endpoint to
	 * delete labels from an issue in bulk. So, just iterate over the
	 * given labels and delete them one after another. */
	char **ids = label_names_to_ids(owner, repo, labels, labels_size);

	for (size_t i = 0; i < labels_size; ++i) {
		char              *url    = NULL;
		gcli_fetch_buffer  buffer = {0};

		url = sn_asprintf("%s/repos/%s/%s/issues/%d/labels/%s",
		                  gitea_get_apibase(), owner, repo, issue, ids[i]);
		gcli_fetch_with_method("DELETE", url, NULL, NULL, &buffer);

		free(buffer.data);
		free(url);
	}

	free_id_list(ids, labels_size);
}
