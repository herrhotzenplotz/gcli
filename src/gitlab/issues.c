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
#include <gcli/gitlab/issues.h>
#include <gcli/json_gen.h>
#include <gcli/json_util.h>

#include <templates/gitlab/issues.h>

#include <pdjson/pdjson.h>

/** Given the url fetch issues */
int
gitlab_fetch_issues(gcli_ctx *ctx, char *url, int const max,
                    gcli_issue_list *const out)
{
	gcli_fetch_list_ctx fl = {
		.listp = &out->issues,
		.sizep = &out->issues_size,
		.max = max,
		.parse = (parsefn)(parse_gitlab_issues),
	};

	return gcli_fetch_list(ctx, url, &fl);
}

int
gitlab_get_issues(gcli_ctx *ctx, char const *owner, char const *repo,
                  gcli_issue_fetch_details const *details, int const max,
                  gcli_issue_list *const out)
{
	char *url = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	char *e_author = NULL;
	char *e_labels = NULL;
	char *e_milestone = NULL;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	if (details->author) {
		char *tmp = gcli_urlencode(details->author);
		e_author = sn_asprintf("%cauthor_username=%s",
		                       details->all ? '?' : '&',
		                       tmp);
		free(tmp);
	}

	if (details->label) {
		char *tmp = gcli_urlencode(details->label);
		int const should_do_qmark = details->all && !details->author;

		e_labels = sn_asprintf("%clabels=%s", should_do_qmark ? '?' : '&', tmp);
		free(tmp);
	}

	if (details->milestone) {
		char *tmp = gcli_urlencode(details->milestone);
		int const should_do_qmark = details->all && !details->author &&
		                            !details->label;

		e_milestone = sn_asprintf("%cmilestone=%s", should_do_qmark ? '?' : '&',
		                          tmp);
		free(tmp);
	}

	url = sn_asprintf("%s/projects/%s%%2F%s/issues%s%s%s%s",
	                  gcli_get_apibase(ctx),
	                  e_owner, e_repo, details->all ? "" : "?state=opened",
	                  e_author ? e_author : "", e_labels ? e_labels : "",
	                  e_milestone ? e_milestone : "");

 	free(e_milestone);
 	free(e_author);
	free(e_labels);
	free(e_owner);
	free(e_repo);

	return gitlab_fetch_issues(ctx, url, max, out);
}

int
gitlab_get_issue_summary(gcli_ctx *ctx, char const *owner, char const *repo,
                         gcli_id const issue_number, gcli_issue *const out)
{
	char *url = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	gcli_fetch_buffer buffer = {0};
	json_stream parser = {0};
	int rc = 0;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/issues/%"PRIid, gcli_get_apibase(ctx),
	                  e_owner, e_repo, issue_number);

	rc = gcli_fetch(ctx, url, NULL, &buffer);
	if (rc == 0) {
		json_open_buffer(&parser, buffer.data, buffer.length);
		json_set_streaming(&parser, true);
		parse_gitlab_issue(ctx, &parser, out);
		json_close(&parser);
	}

	free(url);
	free(e_owner);
	free(e_repo);
	free(buffer.data);

	return rc;
}

int
gitlab_issue_close(gcli_ctx *ctx, char const *owner, char const *repo,
                   gcli_id const issue_number)
{
	char *url = NULL;
	char *data = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	int rc = 0;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url  = sn_asprintf("%s/projects/%s%%2F%s/issues/%"PRIid, gcli_get_apibase(ctx),
	                   e_owner, e_repo, issue_number);
	data = sn_asprintf("{ \"state_event\": \"close\"}");

	rc = gcli_fetch_with_method(ctx, "PUT", url, data, NULL, NULL);

	free(data);
	free(url);
	free(e_owner);
	free(e_repo);

	return rc;
}

int
gitlab_issue_reopen(gcli_ctx *ctx, char const *owner, char const *repo,
                    gcli_id const issue_number)
{
	char *url = NULL;
	char *data = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	int rc = 0;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/issues/%"PRIid, gcli_get_apibase(ctx),
	                  e_owner, e_repo, issue_number);
	data = sn_asprintf("{ \"state_event\": \"reopen\"}");

	rc = gcli_fetch_with_method(ctx, "PUT", url, data, NULL, NULL);

	free(data);
	free(url);
	free(e_owner);
	free(e_repo);

	return rc;
}

int
gitlab_perform_submit_issue(gcli_ctx *ctx, gcli_submit_issue_options opts,
                            gcli_fetch_buffer *const out)
{
	char *e_owner = gcli_urlencode(opts.owner);
	char *e_repo = gcli_urlencode(opts.repo);
	sn_sv e_title = gcli_json_escape(opts.title);
	sn_sv e_body = gcli_json_escape(opts.body);
	int rc = 0;

	char *post_fields = sn_asprintf(
		"{ \"title\": \""SV_FMT"\", \"description\": \""SV_FMT"\" }",
		SV_ARGS(e_title), SV_ARGS(e_body));
	char *url = sn_asprintf("%s/projects/%s%%2F%s/issues", gcli_get_apibase(ctx),
	                        e_owner, e_repo);

	rc = gcli_fetch_with_method(ctx, "POST", url, post_fields, NULL, out);

	free(e_owner);
	free(e_repo);
	free(e_title.data);
	free(e_body.data);
	free(post_fields);
	free(url);

	return rc;
}

int
gitlab_issue_assign(gcli_ctx *ctx, char const *owner, char const *repo,
                    gcli_id const issue_number, char const *assignee)
{
	int assignee_uid = -1;
	char *url = NULL;
	char *post_data = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	int rc = 0;

	assignee_uid = gitlab_user_id(ctx, assignee);
	if (assignee_uid < 0)
		return assignee_uid;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/issues/%"PRIid, gcli_get_apibase(ctx),
	                  e_owner, e_repo, issue_number);
	post_data = sn_asprintf("{ \"assignee_ids\": [ %d ] }", assignee_uid);
	rc = gcli_fetch_with_method(ctx, "PUT", url, post_data, NULL, NULL);

	free(e_owner);
	free(e_repo);
	free(url);
	free(post_data);

	return rc;
}

int
gitlab_issue_add_labels(gcli_ctx *ctx, char const *owner, char const *repo,
                        gcli_id const issue, char const *const labels[],
                        size_t const labels_size)
{
	char *url = NULL;
	char *data = NULL;
	char *list = NULL;
	int rc = 0;

	url = sn_asprintf("%s/projects/%s%%2F%s/issues/%"PRIid, gcli_get_apibase(ctx),
	                  owner, repo, issue);

	list = sn_join_with(labels, labels_size, ",");
	data = sn_asprintf("{ \"add_labels\": \"%s\"}", list);

	rc = gcli_fetch_with_method(ctx, "PUT", url, data, NULL, NULL);

	free(url);
	free(data);
	free(list);

	return rc;
}

int
gitlab_issue_remove_labels(gcli_ctx *ctx, char const *owner, char const *repo,
                           gcli_id const issue, char const *const labels[],
                           size_t const labels_size)
{
	char *url = NULL;
	char *data = NULL;
	char *list = NULL;
	int rc = 0;

	url = sn_asprintf("%s/projects/%s%%2F%s/issues/%"PRIid, gcli_get_apibase(ctx),
	                  owner, repo, issue);

	list = sn_join_with(labels, labels_size, ",");
	data = sn_asprintf("{ \"remove_labels\": \"%s\"}", list);

	rc = gcli_fetch_with_method(ctx, "PUT", url, data, NULL, NULL);

	free(url);
	free(data);
	free(list);

	return rc;
}

int
gitlab_issue_set_milestone(gcli_ctx *ctx, char const *const owner,
                           char const *const repo, gcli_id const issue,
                           gcli_id const milestone)
{
	char *url, *e_owner, *e_repo;
	int rc = 0;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);
	url = sn_asprintf("%s/projects/%s%%2F%s/issues/%"PRIid"?milestone_id=%"PRIid,
	                  gcli_get_apibase(ctx), e_owner, e_repo, issue, milestone);

	rc = gcli_fetch_with_method(ctx, "PUT", url, NULL, NULL, NULL);

	free(url);
	free(e_repo);
	free(e_owner);

	return rc;
}

int
gitlab_issue_clear_milestone(gcli_ctx *ctx, char const *const owner,
                             char const *const repo, gcli_id const issue)
{
	char *url, *e_owner, *e_repo, *payload;
	int rc;

	/* The Gitlab API says:
	 *
	 *    milestone_id: The global ID of a milestone to assign the
	 *    issue to. Set to 0 or provide an empty value to unassign a
	 *    milestone.
	 *
	 * However, the documentation is plain wrong and trying to set it
	 * to zero does absolutely nothing. What do you expect from an
	 * enterprise quality product?! Certainly not this kind of spanish
	 * inquisition. Fear and surprise. That's the Gitlab API in a
	 * nutshell.*/

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);
	url = sn_asprintf("%s/projects/%s%%2F%s/issues/%"PRIid, gcli_get_apibase(ctx),
	                  e_owner, e_repo, issue);
	payload = sn_asprintf("{ \"milestone_id\": null }");

	rc = gcli_fetch_with_method(ctx, "PUT", url, payload, NULL, NULL);

	free(payload);
	free(url);
	free(e_repo);
	free(e_owner);

	return rc;
}

int
gitlab_issue_set_title(gcli_ctx *ctx, char const *owner, char const *repo,
                       gcli_id issue, char const *const new_title)
{
	char *url, *e_owner, *e_repo, *payload;
	gcli_jsongen gen = {0};
	int rc;

	/* Generate url */
	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/projects/%s%%2F%s/issues/%"PRIid,
	                  gcli_get_apibase(ctx), e_owner, e_repo, issue);
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

	rc = gcli_fetch_with_method(ctx, "PUT", url, payload, NULL, NULL);

	free(url);
	free(payload);

	return rc;
}
