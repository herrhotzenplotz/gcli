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

#include <gcli/curl.h>
#include <gcli/gitlab/api.h>
#include <gcli/gitlab/config.h>
#include <gcli/gitlab/repos.h>
#include <gcli/gitlab/merge_requests.h>
#include <gcli/json_util.h>
#include <gcli/json_gen.h>

#include <templates/gitlab/merge_requests.h>

#include <pdjson/pdjson.h>

/* Workaround because gitlab doesn't give us an explicit field for
 * this. */
static void
gitlab_mrs_fixup(gcli_pull_list *const list)
{
	for (size_t i = 0; i < list->pulls_size; ++i) {
		list->pulls[i].merged = !strcmp(list->pulls[i].state, "merged");
	}
}

int
gitlab_fetch_mrs(gcli_ctx *ctx, char *url, int const max,
                 gcli_pull_list *const list)
{
	int rc = 0;

	gcli_fetch_list_ctx fl = {
		.listp = &list->pulls,
		.sizep = &list->pulls_size,
		.max = max,
		.parse = (parsefn)(parse_gitlab_mrs),
	};

	rc = gcli_fetch_list(ctx, url, &fl);

	/* TODO: don't leak the list on error */
	if (rc == 0)
		gitlab_mrs_fixup(list);

	return rc;
}

int
gitlab_get_mrs(gcli_ctx *ctx, char const *owner, char const *repo,
               gcli_pull_fetch_details const *const details, int const max,
               gcli_pull_list *const list)
{
	char *url = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	char *e_author = NULL;
	char *e_label = NULL;
	char *e_milestone = NULL;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	if (details->author) {
		char *tmp = gcli_urlencode(details->author);
		bool const need_qmark = details->all;
		e_author = sn_asprintf("%cauthor_username=%s", need_qmark ? '?' : '&', tmp);
		free(tmp);
	}

	if (details->label) {
		char *tmp = gcli_urlencode(details->label);
		bool const need_qmark = details->all && !details->author;
		e_label = sn_asprintf("%clabels=%s", need_qmark ? '?' : '&', tmp);
		free(tmp);
	}

	if (details->milestone) {
		char *tmp = gcli_urlencode(details->milestone);
		bool const need_qmark = details->all && !details->author && !details->label;
		e_milestone = sn_asprintf("%cmilestone=%s", need_qmark ? '?' : '&', tmp);
		free(tmp);
	}

	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests%s%s%s%s",
	                  gcli_get_apibase(ctx),
	                  e_owner, e_repo,
	                  details->all ? "" : "?state=opened",
	                  e_author ? e_author : "",
	                  e_label ? e_label : "",
	                  e_milestone ? e_milestone : "");

	free(e_milestone);
	free(e_label);
	free(e_author);
	free(e_owner);
	free(e_repo);

	return gitlab_fetch_mrs(ctx, url, max, list);
}

static void
gitlab_free_diff(gitlab_diff *diff)
{
	free(diff->diff);
	free(diff->old_path);
	free(diff->new_path);
	free(diff->a_mode);
	free(diff->b_mode);

	memset(diff, 0, sizeof(*diff));
}

static void
gitlab_free_diffs(gitlab_diff_list *list)
{
	for (size_t i = 0; i < list->diffs_size; ++i) {
		gitlab_free_diff(&list->diffs[i]);
	}

	free(list->diffs);
	list->diffs = NULL;
	list->diffs_size = 0;
}

static int
gitlab_make_commit_patch(gcli_ctx *ctx, FILE *stream,
                         char const *const e_owner, char const *const e_repo,
                         char const *const prev_commit_sha,
                         gcli_commit const *const commit)
{
	char *url;
	int rc;
	gitlab_diff_list list = {0};

	gcli_fetch_list_ctx fl = {
		.listp = &list.diffs,
		.sizep = &list.diffs_size,
		.max = -1,
		.parse = (parsefn)(parse_gitlab_diffs),
	};

	/* /projects/:id/repository/commits/:sha/diff */
	url = sn_asprintf("%s/projects/%s%%2F%s/repository/commits/%s/diff",
	                  gcli_get_apibase(ctx), e_owner, e_repo, commit->sha);

	rc = gcli_fetch_list(ctx, url, &fl);
	if (rc < 0)
		goto err_fetch_diffs;

	fprintf(stream, "From %s Mon Sep 17 00:00:00 2001\n", commit->long_sha);
	fprintf(stream, "From: %s <%s>\n", commit->author, commit->email);
	fprintf(stream, "Date: %s\n", commit->date);
	fprintf(stream, "Subject: %s\n\n", commit->message);

	for (size_t i = 0; i < list.diffs_size; ++i) {
		gitlab_diff const *d = &list.diffs[i];
		fprintf(stream,
		        "diff --git a/%s b/%s\n"
		        "index %s..%s %s\n"
		        "--- a/%s\n"
		        "+++ b/%s\n"
		        "%s",
		        d->old_path, d->new_path,
		        prev_commit_sha, commit->sha, d->b_mode,
		        d->old_path, d->new_path,
		        d->diff);
	}

	fprintf(stream, "--\n2.42.2\n\n");

	gitlab_free_diffs(&list);

err_fetch_diffs:

	return rc;
}

int
gitlab_mr_get_patch(gcli_ctx *ctx, FILE *stream, char const *owner,
                    char const *reponame, gcli_id mr_number)
{
	int rc = 0;
	char *e_owner, *e_repo;
	gcli_pull pull = {0};
	gcli_commit_list commits = {0};
	char const *prev_commit_sha;
	char *base_sha_short;

	rc = gitlab_get_pull(ctx, owner, reponame, mr_number, &pull);
	if (rc < 0)
		goto err_get_pull;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(reponame);

	rc = gitlab_get_pull_commits(ctx, owner, reponame, mr_number, &commits);
	if (rc < 0)
		goto err_get_commit_list;

	base_sha_short = sn_strndup(pull.base_sha, 8);
	prev_commit_sha = base_sha_short;
	for (size_t i = 0; i < commits.commits_size; ++i) {
		rc = gitlab_make_commit_patch(ctx, stream, e_owner, e_repo,
		                              prev_commit_sha,
		                              &commits.commits[i]);
		if (rc < 0)
			goto err_make_commit_patch;

		prev_commit_sha = commits.commits[i].sha;
	}

err_make_commit_patch:
	free(base_sha_short);
	gcli_commits_free(&commits);

err_get_commit_list:
	free(e_owner);
	free(e_repo);

err_get_pull:
	return rc;
}

int
gitlab_mr_get_diff(gcli_ctx *ctx, FILE *stream, char const *owner,
                   char const *reponame, gcli_id mr_number)
{
	(void) stream;
	(void) owner;
	(void) reponame;
	(void) mr_number;

	return gcli_error(ctx, "not yet implemented");
}

int
gitlab_mr_merge(gcli_ctx *ctx, char const *owner, char const *repo,
                gcli_id const mr_number, enum gcli_merge_flags const flags)
{
	gcli_fetch_buffer  buffer  = {0};
	char *url = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	char const *data = "{}";
	bool const squash = flags & GCLI_PULL_MERGE_SQUASH;
	bool const delete_source = flags & GCLI_PULL_MERGE_DELETEHEAD;
	int rc = 0;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	/* PUT /projects/:id/merge_requests/:merge_request_iid/merge */
	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests/%"PRIid"/merge"
	                  "?squash=%s"
	                  "&should_remove_source_branch=%s",
	                  gcli_get_apibase(ctx),
	                  e_owner, e_repo, mr_number,
	                  squash ? "true" : "false",
	                  delete_source ? "true" : "false");

	rc = gcli_fetch_with_method(ctx, "PUT", url, data, NULL, &buffer);

	free(buffer.data);
	free(url);
	free(e_owner);
	free(e_repo);

	return rc;
}

int
gitlab_get_pull(gcli_ctx *ctx, char const *owner, char const *repo,
                gcli_id const pr_number, gcli_pull *const out)
{
	gcli_fetch_buffer json_buffer = {0};
	char *url = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	int rc = 0;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	/* GET /projects/:id/merge_requests/:merge_request_iid */
	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests/%"PRIid,
	                  gcli_get_apibase(ctx),
	                  e_owner, e_repo, pr_number);
	free(e_owner);
	free(e_repo);

	rc = gcli_fetch(ctx, url, NULL, &json_buffer);
	if (rc == 0) {
		json_stream stream = {0};
		json_open_buffer(&stream, json_buffer.data, json_buffer.length);
		parse_gitlab_mr(ctx, &stream, out);
		json_close(&stream);
	}

	free(url);
	free(json_buffer.data);

	return rc;
}

int
gitlab_get_pull_commits(gcli_ctx *ctx, char const *owner, char const *repo,
                        gcli_id const pr_number, gcli_commit_list *const out)
{
	char *url = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;

	gcli_fetch_list_ctx fl = {
		.listp = &out->commits,
		.sizep = &out->commits_size,
		.max = -1,
		.parse = (parsefn)(parse_gitlab_commits),
	};

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	/* GET /projects/:id/merge_requests/:merge_request_iid/commits */
	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests/%"PRIid"/commits",
	                  gcli_get_apibase(ctx), e_owner, e_repo, pr_number);

	free(e_owner);
	free(e_repo);

	return gcli_fetch_list(ctx, url, &fl);
}

int
gitlab_mr_close(gcli_ctx *ctx, char const *owner, char const *repo,
                gcli_id const pr_number)
{
	char *url = NULL;
	char *data = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	int rc = 0;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests/%"PRIid,
	                  gcli_get_apibase(ctx), e_owner, e_repo, pr_number);
	data = sn_asprintf("{ \"state_event\": \"close\"}");

	rc = gcli_fetch_with_method(ctx, "PUT", url, data, NULL, NULL);

	free(url);
	free(e_owner);
	free(e_repo);
	free(data);

	return rc;
}

int
gitlab_mr_reopen(gcli_ctx *ctx, char const *owner, char const *repo,
                 gcli_id const pr_number)
{
	char *url = NULL;
	char *data = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	int rc = 0;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests/%"PRIid,
	                  gcli_get_apibase(ctx), e_owner, e_repo, pr_number);
	data = sn_asprintf("{ \"state_event\": \"reopen\"}");

	rc = gcli_fetch_with_method(ctx, "PUT", url, data, NULL, NULL);

	free(e_owner);
	free(e_repo);
	free(url);
	free(data);

	return rc;
}

int
gitlab_perform_submit_mr(gcli_ctx *ctx, gcli_submit_pull_options opts)
{
	/* Note: this doesn't really allow merging into repos with
	 * different names. We need to figure out a way to make this
	 * better for both github and gitlab. */
	gcli_repo target = {0};
	sn_sv target_branch = {0};
	sn_sv source_owner = {0};
	sn_sv source_branch = {0};
	char *labels = NULL;
	int rc = 0;

	/* json escaped variants */
	sn_sv e_source_branch, e_target_branch, e_title, e_body;

	target_branch = opts.to;
	source_branch = opts.from;
	source_owner = sn_sv_chop_until(&source_branch, ':');
	if (source_branch.length == 0)
		return gcli_error(ctx, "bad merge request source: expected 'owner:branch'");

	source_branch.length -= 1;
	source_branch.data += 1;

	/* Figure out the project id */
	rc = gitlab_get_repo(ctx, opts.owner, opts.repo, &target);
	if (rc < 0)
		return rc;

	/* escape things in the post payload */
	e_source_branch = gcli_json_escape(source_branch);
	e_target_branch = gcli_json_escape(target_branch);
	e_title = gcli_json_escape(opts.title);
	e_body = gcli_json_escape(opts.body);

	/* Prepare the label list if needed */
	if (opts.labels_size) {
		char *joined_items = NULL;

		/* Join by "," */
		joined_items = sn_join_with(
			opts.labels, opts.labels_size, "\",\"");

		/* Construct something we can shove into the payload below */
		labels = sn_asprintf(", \"labels\": [\"%s\"]", joined_items);
		free(joined_items);
	}

	/* prepare payload */
	char *post_fields = sn_asprintf(
		"{\"source_branch\":\""SV_FMT"\",\"target_branch\":\""SV_FMT"\", "
		"\"title\": \""SV_FMT"\", \"description\": \""SV_FMT"\", "
		"\"target_project_id\": %"PRIid" %s }",
		SV_ARGS(e_source_branch),
		SV_ARGS(e_target_branch),
		SV_ARGS(e_title),
		SV_ARGS(e_body),
		target.id,
		labels ? labels : "");

	/* construct url. The thing below works as the string view is
	 * malloced and also NUL-terminated */
	char *e_owner = gcli_urlencode_sv(source_owner).data;
	char *e_repo = gcli_urlencode(opts.repo);

	char *url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests",
	                        gcli_get_apibase(ctx),
	    e_owner, e_repo);

	/* perform request */
	rc = gcli_fetch_with_method(ctx, "POST", url, post_fields, NULL, NULL);

	/* cleanup */
	free(e_source_branch.data);
	free(e_target_branch.data);
	free(e_title.data);
	free(e_body.data);
	free(e_owner);
	free(e_repo);
	free(labels);
	free(post_fields);
	free(url);

	return rc;
}

int
gitlab_mr_add_labels(gcli_ctx *ctx, char const *owner, char const *repo,
                     gcli_id const mr, char const *const labels[],
                     size_t const labels_size)
{
	char *url  = NULL;
	char *data = NULL;
	char *list = NULL;
	int   rc   = 0;

	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests/%"PRIid,
	                  gcli_get_apibase(ctx), owner, repo, mr);

	list = sn_join_with(labels, labels_size, ",");
	data = sn_asprintf("{ \"add_labels\": \"%s\"}", list);

	rc = gcli_fetch_with_method(ctx, "PUT", url, data, NULL, NULL);

	free(url);
	free(data);
	free(list);

	return rc;
}

int
gitlab_mr_remove_labels(gcli_ctx *ctx, char const *owner, char const *repo,
                        gcli_id const mr, char const *const labels[],
                        size_t const labels_size)
{
	char *url = NULL;
	char *data = NULL;
	char *list = NULL;
	int rc = 0;

	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests/%"PRIid,
	                  gcli_get_apibase(ctx), owner, repo, mr);

	list = sn_join_with(labels, labels_size, ",");
	data = sn_asprintf("{ \"remove_labels\": \"%s\"}", list);

	rc = gcli_fetch_with_method(ctx, "PUT", url, data, NULL, NULL);

	free(url);
	free(data);
	free(list);

	return rc;
}

int
gitlab_mr_set_milestone(gcli_ctx *ctx, char const *owner, char const *repo,
                        gcli_id mr, gcli_id milestone_id)
{
	char *url = NULL;
	char *data = NULL;
	int rc = 0;

	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests/%"PRIid,
	                  gcli_get_apibase(ctx), owner, repo, mr);

	data = sn_asprintf("{ \"milestone_id\": \"%"PRIid"\"}", milestone_id);

	rc = gcli_fetch_with_method(ctx, "PUT", url, data, NULL, NULL);

	free(url);
	free(data);

	return rc;
}

int
gitlab_mr_clear_milestone(gcli_ctx *ctx, char const *owner, char const *repo,
                          gcli_id const mr)
{
	/* GitLab's REST API docs state:
	 *
	 * The global ID of a milestone to assign the merge request
	 * to. Set to 0 or provide an empty value to unassign a
	 * milestone. */
	return gitlab_mr_set_milestone(ctx, owner, repo, mr, 0);
}

/* Helper function to fetch the list of user ids that are reviewers
 * of a merge requests. */
static int
gitlab_mr_get_reviewers(gcli_ctx *ctx, char const *e_owner, char const *e_repo,
                        gcli_id const mr, gitlab_reviewer_id_list *const out)
{
	char *url;
	int rc;
	gcli_fetch_buffer json_buffer = {0};

	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests/%"PRIid,
	                  gcli_get_apibase(ctx), e_owner, e_repo, mr);

	rc = gcli_fetch(ctx, url, NULL, &json_buffer);
	if (rc == 0) {
		json_stream stream = {0};
		json_open_buffer(&stream, json_buffer.data, json_buffer.length);
		parse_gitlab_reviewer_ids(ctx, &stream, out);
		json_close(&stream);
	}

	free(url);
	free(json_buffer.data);

	return rc;
}

static void
gitlab_reviewer_list_free(gitlab_reviewer_id_list *const list)
{
	free(list->reviewers);
	list->reviewers = NULL;
	list->reviewers_size = 0;
}

int
gitlab_mr_add_reviewer(gcli_ctx *ctx, char const *owner, char const *repo,
                       gcli_id mr_number, char const *username)
{
	char *url, *e_owner, *e_repo, *payload;
	int uid, rc = 0;
	gitlab_reviewer_id_list list = {0};
	gcli_jsongen gen = {0};

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	/* Fetch list of already existing reviewers */
	rc = gitlab_mr_get_reviewers(ctx, e_owner, e_repo, mr_number, &list);
	if (rc < 0)
		goto bail_get_reviewers;

	/* Resolve user id from user name */
	uid = gitlab_user_id(ctx, username);
	if (uid < 0)
		goto bail_resolve_user_id;

	/* Start generating payload */
	gcli_jsongen_init(&gen);
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, "reviewer_ids");

		gcli_jsongen_begin_array(&gen);
		{
			for (size_t i = 0; i < list.reviewers_size; ++i)
				gcli_jsongen_number(&gen, list.reviewers[i]);

			/* Push new user id into list of user ids */
			gcli_jsongen_number(&gen, uid);
		}
		gcli_jsongen_end_array(&gen);
	}
	gcli_jsongen_end_object(&gen);


	payload = gcli_jsongen_to_string(&gen);
	gcli_jsongen_free(&gen);

	/* generate URL */
	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests/%"PRIid,
	                  gcli_get_apibase(ctx), e_owner, e_repo, mr_number);

	rc = gcli_fetch_with_method(ctx, "PUT", url, payload, NULL, NULL);

	free(url);
	free(payload);

bail_resolve_user_id:
	gitlab_reviewer_list_free(&list);

bail_get_reviewers:
	free(e_owner);
	free(e_repo);

	return rc;
}

int
gitlab_mr_set_title(gcli_ctx *ctx, char const *const owner,
                    char const *const repo, gcli_id const id,
                    char const *const new_title)
{
	char *url, *e_owner, *e_repo, *payload;
	gcli_jsongen gen = {0};
	int rc = 0;

	/* Generate url
	 *
	 * PUT /projects/:id/merge_requests/:merge_request_iid */
	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests/%"PRIid,
	                  gcli_get_apibase(ctx), e_owner, e_repo, id);
	free(e_owner);
	free(e_repo);

	/* Generate payload */
	gcli_jsongen_init(&gen);
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, "title");
		gcli_jsongen_string(&gen, new_title);
	}
	gcli_jsongen_end_object(&gen);

	payload = gcli_jsongen_to_string(&gen);
	gcli_jsongen_free(&gen);

	/* perform request */
	rc = gcli_fetch_with_method(ctx, "PUT", url, payload, NULL, NULL);

	/* clean up */
	free(url);
	free(payload);

	return rc;
}
