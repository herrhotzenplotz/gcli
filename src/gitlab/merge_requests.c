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
#include <gcli/gitlab/config.h>
#include <gcli/gitlab/repos.h>
#include <gcli/gitlab/merge_requests.h>
#include <gcli/json_util.h>

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
gitlab_fetch_mrs(char *url, int const max, gcli_pull_list *const list)
{
	json_stream        stream      = {0};
	gcli_fetch_buffer  json_buffer = {0};
	char              *next_url    = NULL;

	do {
		gcli_fetch(url, &next_url, &json_buffer);
		json_open_buffer(&stream, json_buffer.data, json_buffer.length);
		parse_gitlab_mrs(&stream, &list->pulls, &list->pulls_size);

		free(json_buffer.data);
		free(url);
		json_close(&stream);
	} while ((url = next_url) && (max == -1 || (int)list->pulls_size < max));

	free(url);

	gitlab_mrs_fixup(list);

	return 0;
}

int
gitlab_get_mrs(char const *owner,
               char const *repo,
               gcli_pull_fetch_details const *const details,
               int const max,
               gcli_pull_list *const list)
{
	char *url = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	char *e_author = NULL;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	if (details->author) {
		char *tmp = gcli_urlencode(details->author);
		e_author = sn_asprintf("?author_username=%s", tmp);
		free(tmp);
	}

	/* Note that Gitlab gives you all merge requests inclduing merged
	 * and closed ones by default. If we want only open merge
	 * requests, we have to be very explicit about it and give it the
	 * state=open flag for this purpose.
	 *
	 * The below ternary stuff may look like magic, however it is just
	 * to make sure we insert the right character into the param
	 * list. Again, note that this behaviour differs quite
	 * substantially from Github's. */
	url = sn_asprintf(
		"%s/projects/%s%%2F%s/merge_requests%s%s%s",
		gitlab_get_apibase(),
		e_owner, e_repo,
		e_author ? e_author : "",
		e_author ? (details->all ? "" : "&") : (details->all ? "" : "?"),
		details->all ? "" : "state=opened");

	free(e_author);
	free(e_owner);
	free(e_repo);

	return gitlab_fetch_mrs(url, max, list);
}

void
gitlab_print_pr_diff(FILE *stream,
                     char const *owner,
                     char const *repo,
                     int const pr_number)
{
	(void)owner;
	(void)repo;
	(void)pr_number;

	fprintf(stream,
	        "note : Getting the diff of a Merge Request is not "
	        "supported on GitLab. Blame the Gitlab people.\n");
}

int
gitlab_mr_merge(char const *owner,
                char const *repo,
                int const mr_number,
                enum gcli_merge_flags const flags)
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
	url = sn_asprintf(
		"%s/projects/%s%%2F%s/merge_requests/%d/merge"
		"?squash=%s"
		"&should_remove_source_branch=%s",
		gitlab_get_apibase(),
		e_owner, e_repo, mr_number,
		squash ? "true" : "false",
		delete_source ? "true" : "false");

	rc = gcli_fetch_with_method("PUT", url, data, NULL, &buffer);

	/* if verbose or normal noise level, print the url */
	if (rc == 0 && !sn_quiet())
		gcli_print_html_url(buffer);

	free(buffer.data);
	free(url);
	free(e_owner);
	free(e_repo);

	return rc;
}

void
gitlab_get_pull(char const *owner,
                char const *repo,
                int const pr_number,
                gcli_pull *const out)
{
	json_stream        stream      = {0};
	gcli_fetch_buffer  json_buffer = {0};
	char              *url         = NULL;
	char              *e_owner     = NULL;
	char              *e_repo      = NULL;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	/* GET /projects/:id/merge_requests/:merge_request_iid */
	url = sn_asprintf(
		"%s/projects/%s%%2F%s/merge_requests/%d",
		gitlab_get_apibase(),
		e_owner, e_repo, pr_number);
	gcli_fetch(url, NULL, &json_buffer);

	json_open_buffer(&stream, json_buffer.data, json_buffer.length);

	parse_gitlab_mr(&stream, out);

	json_close(&stream);
	free(url);
	free(e_owner);
	free(e_repo);
	free(json_buffer.data);
}

int
gitlab_get_pull_commits(char const *owner,
                        char const *repo,
                        int const pr_number,
                        gcli_commit **const out)
{
	char              *url         = NULL;
	char              *next_url    = NULL;
	char              *e_owner     = NULL;
	char              *e_repo      = NULL;
	size_t             count       = 0;
	json_stream        stream      = {0};
	gcli_fetch_buffer  json_buffer = {0};

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	/* GET /projects/:id/merge_requests/:merge_request_iid/commits */
	url = sn_asprintf(
		"%s/projects/%s%%2F%s/merge_requests/%d/commits",
		gitlab_get_apibase(),
		e_owner, e_repo, pr_number);

	do {
		gcli_fetch(url, &next_url, &json_buffer);
		json_open_buffer(&stream, json_buffer.data, json_buffer.length);

		parse_gitlab_commits(&stream, out, &count);

		json_close(&stream);
		free(url);
		free(json_buffer.data);
	} while ((url = next_url));

	free(e_owner);
	free(e_repo);

	return (int)(count);
}

int
gitlab_mr_close(char const *owner, char const *repo, int const pr_number)
{
	char *url = NULL;
	char *data = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	int rc = 0;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url  = sn_asprintf(
		"%s/projects/%s%%2F%s/merge_requests/%d",
		gitlab_get_apibase(),
		e_owner, e_repo, pr_number);
	data = sn_asprintf("{ \"state_event\": \"close\"}");

	rc = gcli_fetch_with_method("PUT", url, data, NULL, NULL);

	free(url);
	free(e_owner);
	free(e_repo);
	free(data);

	return rc;
}

int
gitlab_mr_reopen(char const *owner, char const *repo, int const pr_number)
{
	char *url = NULL;
	char *data = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	int rc = 0;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url  = sn_asprintf(
		"%s/projects/%s%%2F%s/merge_requests/%d",
		gitlab_get_apibase(),
		e_owner, e_repo, pr_number);
	data = sn_asprintf("{ \"state_event\": \"reopen\"}");

	rc = gcli_fetch_with_method("PUT", url, data, NULL, NULL);

	free(e_owner);
	free(e_repo);
	free(url);
	free(data);

	return rc;
}

int
gitlab_perform_submit_mr(gcli_submit_pull_options opts)
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

	target_branch         = opts.to;
	source_branch         = opts.from;
	source_owner          = sn_sv_chop_until(&source_branch, ':');
	source_branch.length -= 1;
	source_branch.data   += 1;

	/* Figure out the project id */
	rc = gitlab_get_repo(opts.owner, opts.repo, &target);
	if (rc < 0)
		return rc;

	/* escape things in the post payload */
	e_source_branch = gcli_json_escape(source_branch);
	e_target_branch = gcli_json_escape(target_branch);
	e_title         = gcli_json_escape(opts.title);
	e_body          = gcli_json_escape(opts.body);

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
		"\"target_project_id\": %d %s }",
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

	char *url = sn_asprintf(
		"%s/projects/%s%%2F%s/merge_requests",
		gitlab_get_apibase(),
	    e_owner, e_repo);

	/* perform request */
	rc = gcli_fetch_with_method("POST", url, post_fields, NULL, NULL);

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
gitlab_mr_add_labels(char const *owner,
                     char const *repo,
                     int const mr,
                     char const *const labels[],
                     size_t const labels_size)
{
	char *url  = NULL;
	char *data = NULL;
	char *list = NULL;
	int   rc   = 0;

	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests/%d",
	                  gitlab_get_apibase(), owner, repo, mr);

	list = sn_join_with(labels, labels_size, ",");
	data = sn_asprintf("{ \"add_labels\": \"%s\"}", list);

	rc = gcli_fetch_with_method("PUT", url, data, NULL, NULL);

	free(url);
	free(data);
	free(list);

	return rc;
}

int
gitlab_mr_remove_labels(char const *owner,
                        char const *repo,
                        int const mr,
                        char const *const labels[],
                        size_t const labels_size)
{
	char *url  = NULL;
	char *data = NULL;
	char *list = NULL;
	int   rc   = 0;

	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests/%d",
	                  gitlab_get_apibase(), owner, repo, mr);

	list = sn_join_with(labels, labels_size, ",");
	data = sn_asprintf("{ \"remove_labels\": \"%s\"}", list);

	rc = gcli_fetch_with_method("PUT", url, data, NULL, NULL);

	free(url);
	free(data);
	free(list);

	return rc;
}

int
gitlab_mr_set_milestone(char const *owner,
                        char const *repo,
                        int mr,
                        int milestone_id)
{
	char *url  = NULL;
	char *data = NULL;

	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests/%d",
	                  gitlab_get_apibase(), owner, repo, mr);

	data = sn_asprintf("{ \"milestone_id\": \"%d\"}", milestone_id);

	gcli_fetch_with_method("PUT", url, data, NULL, NULL);

	free(url);
	free(data);

	return 0;
}

int
gitlab_mr_clear_milestone(char const *owner,
                          char const *repo,
                          int mr)
{
	/* GitLab's REST API docs state:
	 *
	 * The global ID of a milestone to assign the merge request
	 * to. Set to 0 or provide an empty value to unassign a
	 * milestone. */
	gitlab_mr_set_milestone(owner, repo, mr, 0);

	return 0;
}
