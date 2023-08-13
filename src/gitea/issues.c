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
                 gcli_issue_fetch_details const *details,
                 int const max,
                 gcli_issue_list *const out)
{
	return github_get_issues(owner, repo, details, max, out);
}

int
gitea_get_issue_summary(char const *owner, char const *repo,
                        int const issue_number, gcli_issue *const out)
{
	return github_get_issue_summary(owner, repo, issue_number, out);
}

int
gitea_submit_issue(gcli_submit_issue_options opts,
                   gcli_fetch_buffer *const out)
{
	return github_perform_submit_issue(opts, out);
}

/* Gitea has closed, Github has close ... go figure */
static int
gitea_issue_patch_state(char const *owner,
                        char const *repo,
                        int const issue_number,
                        char const *const state)
{
	char *url     = NULL;
	char *data    = NULL;
	char *e_owner = NULL;
	char *e_repo  = NULL;
	int   rc      = 0;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf(
		"%s/repos/%s/%s/issues/%d",
		gitea_get_apibase(),
		e_owner, e_repo,
		issue_number);
	data = sn_asprintf("{ \"state\": \"%s\"}", state);

	rc = gcli_fetch_with_method("PATCH", url, data, NULL, NULL);

	free(data);
	free(url);
	free(e_owner);
	free(e_repo);

	return rc;
}

int
gitea_issue_close(char const *owner,
                  char const *repo,
                  int const issue_number)
{
	return gitea_issue_patch_state(owner, repo, issue_number, "closed");
}

int
gitea_issue_reopen(char const *owner,
                   char const *repo,
                   int const issue_number)
{
	return gitea_issue_patch_state(owner, repo, issue_number, "open");
}

int
gitea_issue_assign(char const *owner,
                   char const *repo,
                   int const issue_number,
                   char const *const assignee)
{
	sn_sv  escaped_assignee = SV_NULL;
	char  *post_fields      = NULL;
	char  *url              = NULL;
	char  *e_owner          = NULL;
	char  *e_repo           = NULL;
	int    rc               = 0;

	escaped_assignee = gcli_json_escape(SV((char *)assignee));
	post_fields = sn_asprintf("{ \"assignees\": [\""SV_FMT"\"] }",
	                          SV_ARGS(escaped_assignee));

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf(
		"%s/repos/%s/%s/issues/%d",
		gitea_get_apibase(), e_owner, e_repo, issue_number);

	rc = gcli_fetch_with_method("PATCH", url, post_fields, NULL, NULL);

	free(escaped_assignee.data);
	free(post_fields);
	free(e_owner);
	free(e_repo);
	free(url);

	return rc;
}

/* Return the stringified id of the given label */
static char *
get_id_of_label(char const *label_name,
                gcli_label_list const *const list)
{
	for (size_t i = 0; i < list->labels_size; ++i)
		if (strcmp(list->labels[i].name, label_name) == 0)
			return sn_asprintf("%ld", list->labels[i].id);
	return NULL;
}

static char **
label_names_to_ids(char const *owner,
                   char const *repo,
                   char const *const names[],
                   size_t const names_size)
{
	gcli_label_list list = {0};
	char **ids = NULL;
	size_t ids_size = 0;

    gitea_get_labels(owner, repo, -1, &list);

	for (size_t i = 0; i < names_size; ++i) {
		char *const label_id = get_id_of_label(names[i], &list);

		if (!label_id)
			errx(1, "error: no such label '%s'", names[i]);

		ids = realloc(ids, sizeof(*ids) * (ids_size +1));
		ids[ids_size++] = label_id;
	}

	gcli_free_labels(&list);

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

int
gitea_issue_add_labels(char const *owner,
                       char const *repo,
                       int const issue,
                       char const *const labels[],
                       size_t const labels_size)
{
	char *list = NULL;
	char *data = NULL;
	char *url  = NULL;
	int   rc   = 0;

	/* First, convert to ids */
	char **ids = label_names_to_ids(owner, repo, labels, labels_size);

	/* Construct json payload */

	/* Note: http://www.c-faq.com/ansi/constmismatch.html */
	list = sn_join_with((char const **)ids, labels_size, ",");
	data = sn_asprintf("{ \"labels\": [%s] }", list);

	url = sn_asprintf("%s/repos/%s/%s/issues/%d/labels",
	                  gitea_get_apibase(), owner, repo, issue);

	rc = gcli_fetch_with_method("POST", url, data, NULL, NULL);

	free(list);
	free(data);
	free(url);
	free_id_list(ids, labels_size);

	return rc;
}

int
gitea_issue_remove_labels(char const *owner,
                          char const *repo,
                          int const issue,
                          char const *const labels[],
                          size_t const labels_size)
{
	int rc = 0;
	/* Unfortunately the gitea api does not give us an endpoint to
	 * delete labels from an issue in bulk. So, just iterate over the
	 * given labels and delete them one after another. */
	char **ids = label_names_to_ids(owner, repo, labels, labels_size);

	for (size_t i = 0; i < labels_size; ++i) {
		char *url = NULL;

		url = sn_asprintf("%s/repos/%s/%s/issues/%d/labels/%s",
		                  gitea_get_apibase(), owner, repo, issue, ids[i]);
		rc = gcli_fetch_with_method("DELETE", url, NULL, NULL, NULL);

		free(url);

		if (rc < 0)
			break;
	}

	free_id_list(ids, labels_size);

	return rc;
}

int
gitea_issue_set_milestone(char const *const owner,
                          char const *const repo,
                          int const issue,
                          int const milestone)
{
	return github_issue_set_milestone(owner, repo, issue, milestone);
}

int
gitea_issue_clear_milestone(char const *owner,
                            char const *repo,
                            int issue)
{
	return github_issue_set_milestone(owner, repo, issue, 0);
}
