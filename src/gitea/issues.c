/*
 * Copyright 2022-2025 Nico Sonack <nsonack@herrhotzenplotz.de>
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
#include <gcli/gitea/repos.h>
#include <gcli/github/issues.h>
#include <gcli/json_gen.h>
#include <gcli/json_util.h>
#include <gcli/labels.h>
#include <gcli/url.h>

#include <pdjson/pdjson.h>

#include <templates/github/issues.h>

#include <stdarg.h>

#include <gcli/port/string.h>

int
gitea_issue_make_url(struct gcli_ctx *ctx, struct gcli_path const *const path,
                     char **url, char const *const suffix_fmt, ...)
{
	char *suffix = NULL;
	int rc = 0;
	va_list vp;

	va_start(vp, suffix_fmt);
	suffix = gcli_vasprintf(suffix_fmt, vp);
	va_end(vp);

	switch (path->kind) {
	case GCLI_PATH_DEFAULT: {
		char *e_owner = NULL, *e_repo = NULL;

		e_owner = gcli_urlencode(path->as_default.owner);
		e_repo  = gcli_urlencode(path->as_default.repo);

		*url = gcli_asprintf("%s/repos/%s/%s/issues/%"PRIid"%s",
		                     gcli_get_apibase(ctx), e_owner, e_repo,
		                     path->as_default.id, suffix);

		gcli_clear_ptr(&e_owner);
		gcli_clear_ptr(&e_repo);
	} break;
	case GCLI_PATH_URL: {
		*url = gcli_asprintf("%s%s", path->as_url, suffix);
	} break;
	default: {
		rc = gcli_error(ctx, "unsupported path kind for Gitea issues");
	} break;
	}

	gcli_clear_ptr(&suffix);

	return rc;
}

static int
gitea_issues_make_url(struct gcli_ctx *ctx, struct gcli_path const *const path,
                      char const *const suffix, char **url)
{
	return gitea_repo_make_url(ctx, path, url, "/issues%s",
	                           suffix ? suffix : "");
}

int
gitea_issues_search(struct gcli_ctx *ctx, struct gcli_path const *const path,
                    struct gcli_issue_fetch_details const *details,
                    int const max, struct gcli_issue_list *const out)
{
	char *url = NULL, *suffix = NULL;
	int rc = 0;

	struct gcli_fetch_list_ctx fl = {
		.listp = &out->issues,
		.sizep = &out->issues_size,
		.parse = (parsefn)(parse_github_issues),
		.max = max,
	};

	gcli_url_options_append(&suffix, "type", "issues");
	gcli_url_options_append(&suffix, "state", details->all ? "all" : "open");
	gcli_url_options_append(&suffix, "milestones", details->milestone);
	gcli_url_options_append(&suffix, "created_by", details->author);
	gcli_url_options_append(&suffix, "labels", details->label);
	gcli_url_options_append(&suffix, "assigned_by", details->assignee);
	gcli_url_options_append(&suffix, "q", details->search_term);

	rc = gitea_issues_make_url(ctx, path, suffix, &url);

	gcli_clear_ptr(&suffix);

	if (rc < 0)
		return rc;

	return gcli_fetch_list(ctx, url, &fl);
}

int
gitea_get_issue_summary(struct gcli_ctx *ctx, struct gcli_path const *const path,
                        struct gcli_issue *const out)
{
	return github_get_issue_summary(ctx, path, out);
}

int
gitea_submit_issue(struct gcli_ctx *const ctx,
                   struct gcli_submit_issue_options *const opts,
                   struct gcli_issue *const out)
{
	return github_perform_submit_issue(ctx, opts, out);
}

static int
gitea_issue_patch_state(struct gcli_ctx *ctx,
                        struct gcli_path const *const path,
                        char const *const state)
{
	char *url = NULL, *payload = NULL;
	struct gcli_jsongen gen = {0};
	int rc = 0;

	rc = gitea_issue_make_url(ctx, path, &url, "");
	if (rc < 0)
		return rc;

	gcli_jsongen_init(&gen);
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, "state");
		gcli_jsongen_string(&gen, state);
	}
	gcli_jsongen_end_object(&gen);

	payload = gcli_jsongen_to_string(&gen);
	gcli_jsongen_free(&gen);

	rc = gcli_fetch_with_method(ctx, "PATCH", url, payload, NULL, NULL);

	gcli_clear_ptr(&payload);
	gcli_clear_ptr(&url);

	return rc;
}

/* Gitea has closed, Github has close ... go figure */
int
gitea_issue_close(struct gcli_ctx *ctx, struct gcli_path const *const path)
{
	return gitea_issue_patch_state(ctx, path, "closed");
}

int
gitea_issue_reopen(struct gcli_ctx *ctx, struct gcli_path const *const path)
{
	return gitea_issue_patch_state(ctx, path, "open");
}

int
gitea_issue_assign(struct gcli_ctx *ctx,
                   struct gcli_path const *const issue_path,
                   char const *const assignee)
{
	char *url = NULL, *payload = NULL;
	struct gcli_jsongen gen = {0};
	int rc = 0;

	/* Generate URL */
	rc = gitea_issue_make_url(ctx, issue_path, &url, "");
	if (rc < 0)
		return rc;

	/* Generate payload */
	gcli_jsongen_init(&gen);
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, "assignees");
		gcli_jsongen_begin_array(&gen);
		gcli_jsongen_string(&gen, assignee);
		gcli_jsongen_end_array(&gen);
	}
	gcli_jsongen_end_object(&gen);

	payload = gcli_jsongen_to_string(&gen);
	gcli_jsongen_free(&gen);

	rc = gcli_fetch_with_method(ctx, "PATCH", url, payload, NULL, NULL);

	gcli_clear_ptr(&payload);
	gcli_clear_ptr(&url);

	return rc;
}

/* Return the stringified id of the given label */
static char *
get_id_of_label(char const *label_name,
                struct gcli_label_list const *const list)
{
	for (size_t i = 0; i < list->labels_size; ++i)
		if (strcmp(list->labels[i].name, label_name) == 0)
			return gcli_asprintf("%"PRIid, list->labels[i].id);
	return NULL;
}

static void
free_id_list(char *list[], size_t const list_size)
{
	for (size_t i = 0; i < list_size; ++i) {
		gcli_clear_ptr(&list[i]);
	}
	gcli_clear_ptr(&list);
}

static char **
label_names_to_ids(struct gcli_ctx *ctx, struct gcli_path const *const path,
                   char const *const names[], size_t const names_size)
{
	struct gcli_label_list list = {0};
	char **ids = NULL;
	size_t ids_size = 0;

	gitea_get_labels(ctx, path, -1, &list);

	for (size_t i = 0; i < names_size; ++i) {
		char *const label_id = get_id_of_label(names[i], &list);

		if (!label_id) {
			free_id_list(ids, ids_size);
			ids = NULL;
			gcli_error(ctx, "no such label '%s'", names[i]);
			goto out;
		}

		ids = realloc(ids, sizeof(*ids) * (ids_size +1));
		ids[ids_size++] = label_id;
	}

out:
	gcli_free_labels(&list);

	return ids;
}

int
gitea_issue_add_labels(struct gcli_ctx *ctx, struct gcli_path const *const path,
                       char const *const labels[], size_t const labels_size)
{
	char *payload = NULL, *url = NULL;
	struct gcli_jsongen gen = {0};
	int rc = 0;

	/* First, convert to ids */
	char **ids = label_names_to_ids(ctx, path, labels, labels_size);
	if (!ids)
		return -1;

	rc = gitea_issue_make_url(ctx, path, &url, "/labels");
	if (rc < 0) {
		free_id_list(ids, labels_size);
		return rc;
	}

	/* Construct json payload */
	gcli_jsongen_init(&gen);
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, "labels");
		gcli_jsongen_begin_array(&gen);
		for (size_t i = 0; i < labels_size; ++i) {
			gcli_jsongen_string(&gen, ids[i]);
		}
		gcli_jsongen_end_array(&gen);
	}
	gcli_jsongen_end_object(&gen);

	payload = gcli_jsongen_to_string(&gen);
	gcli_jsongen_free(&gen);
	free_id_list(ids, labels_size);

	rc = gcli_fetch_with_method(ctx, "POST", url, payload, NULL, NULL);

	gcli_clear_ptr(&payload);
	gcli_clear_ptr(&url);

	return rc;
}

int
gitea_issue_remove_labels(struct gcli_ctx *ctx,
                          struct gcli_path const *const path,
                          char const *const labels[], size_t const labels_size)
{
	int rc = 0;
	/* Unfortunately the gitea api does not give us an endpoint to
	 * delete labels from an issue in bulk. So, just iterate over the
	 * given labels and delete them one after another. */
	char **ids = label_names_to_ids(ctx, path, labels, labels_size);
	if (!ids)
		return -1;

	for (size_t i = 0; i < labels_size; ++i) {
		char *url = NULL;

		rc = gitea_issue_make_url(ctx, path, &url, "/labels/%s", ids[i]);
		if (rc < 0)
			break;

		rc = gcli_fetch_with_method(ctx, "DELETE", url, NULL, NULL, NULL);

		gcli_clear_ptr(&url);

		if (rc < 0)
			break;
	}

	free_id_list(ids, labels_size);

	return rc;
}

int
gitea_issue_set_milestone(struct gcli_ctx *ctx,
                          struct gcli_path const *const issue_path,
                          gcli_id const milestone)
{
	return github_issue_set_milestone(ctx, issue_path, milestone);
}

int
gitea_issue_clear_milestone(struct gcli_ctx *ctx,
                            struct gcli_path const *const issue_path)
{
	return github_issue_set_milestone(ctx, issue_path, 0);
}

int
gitea_issue_set_title(struct gcli_ctx *ctx,
                      struct gcli_path const *const issue_path,
                      char const *const new_title)
{
	return github_issue_set_title(ctx, issue_path, new_title);
}

int
gitea_issue_set_op(struct gcli_ctx *ctx,
                   struct gcli_path const *const path,
                   char const *const new_op)
{
	return github_issue_set_op(ctx, path, new_op);
}
