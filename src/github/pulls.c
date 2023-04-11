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

#include <gcli/config.h>
#include <gcli/curl.h>
#include <gcli/gitconfig.h>
#include <gcli/github/checks.h>
#include <gcli/github/config.h>
#include <gcli/github/issues.h>
#include <gcli/github/pulls.h>
#include <gcli/json_util.h>

#include <pdjson/pdjson.h>

#include <templates/github/pulls.h>

int
github_fetch_pulls(char *url, int max, gcli_pull_list *const list)
{
	json_stream        stream      = {0};
	gcli_fetch_buffer  json_buffer = {0};
	char              *next_url    = NULL;

	do {
		gcli_fetch(url, &next_url, &json_buffer);
		json_open_buffer(&stream, json_buffer.data, json_buffer.length);
		parse_github_pulls(&stream, &list->pulls, &list->pulls_size);

		free(json_buffer.data);
		free(url);
		json_close(&stream);
	} while ((url = next_url) && (max == -1 || (int)(list->pulls_size) < max));

	free(url);

	return 0;
}

int
github_get_pulls(char const *owner,
                 char const *repo,
                 bool const all,
                 int const max,
                 gcli_pull_list *const list)
{
	char *url     = NULL;
	char *e_owner = NULL;
	char *e_repo  = NULL;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf(
		"%s/repos/%s/%s/pulls?state=%s",
		gcli_get_apibase(),
		e_owner, e_repo, all ? "all" : "open");

	free(e_owner);
	free(e_repo);

	return github_fetch_pulls(url, max, list);
}

void
github_print_pull_diff(FILE *stream,
                       char const *owner,
                       char const *repo,
                       int const pr_number)
{
	char *url     = NULL;
	char *e_owner = NULL;
	char *e_repo  = NULL;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf(
		"%s/repos/%s/%s/pulls/%d",
		gcli_get_apibase(),
		e_owner, e_repo, pr_number);
	gcli_curl(stream, url, "Accept: application/vnd.github.v3.diff");

	free(e_owner);
	free(e_repo);
	free(url);
}

/* TODO: figure out a way to get rid of the 3 consecutive urlencode
 * calls */
static void
github_pull_delete_head_branch(char const *owner,
                               char const *repo,
                               int const pr_number)
{
	gcli_pull pull = {0};
	char *url, *e_owner, *e_repo;
	char const *head_branch;

	github_get_pull(owner, repo, pr_number, &pull);

	head_branch = strchr(pull.head_label, ':');
	head_branch++;

	e_owner = gcli_urlencode(owner);
	e_repo = gcli_urlencode(repo);

	url = sn_asprintf("%s/repos/%s/%s/git/refs/heads/%s",
	                  github_get_apibase(), e_owner, e_repo,
	                  head_branch);

	gcli_fetch_with_method("DELETE", url, NULL, NULL, NULL);

	free(url);
	free(e_owner);
	free(e_repo);
	gcli_pull_free(&pull);
}

void
github_pull_merge(char const *owner,
                  char const *repo,
                  int const pr_number,
                  enum gcli_merge_flags const flags)
{
	char *url = NULL;
	char *e_owner = NULL;
	char *e_repo = NULL;
	char const *data = "{}";
	bool const squash = flags & GCLI_PULL_MERGE_SQUASH;
	bool const delete_source = flags & GCLI_PULL_MERGE_DELETEHEAD;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url = sn_asprintf(
		"%s/repos/%s/%s/pulls/%d/merge?merge_method=%s",
		gcli_get_apibase(),
		e_owner, e_repo, pr_number,
		squash ? "squash" : "merge");

	gcli_fetch_with_method("PUT", url, data, NULL, NULL);

	if (delete_source)
		github_pull_delete_head_branch(owner, repo, pr_number);

	free(url);
	free(e_owner);
	free(e_repo);
}

void
github_pull_close(char const *owner, char const *repo, int const pr_number)
{
	gcli_fetch_buffer  json_buffer = {0};
	char              *url         = NULL;
	char              *e_owner     = NULL;
	char              *e_repo      = NULL;
	char              *data        = NULL;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url  = sn_asprintf(
		"%s/repos/%s/%s/pulls/%d",
		gcli_get_apibase(),
		e_owner, e_repo, pr_number);
	data = sn_asprintf("{ \"state\": \"closed\"}");

	gcli_fetch_with_method("PATCH", url, data, NULL, &json_buffer);

	free(json_buffer.data);
	free(url);
	free(e_repo);
	free(e_owner);
	free(data);
}

void
github_pull_reopen(char const *owner, char const *repo, int const pr_number)
{
	gcli_fetch_buffer  json_buffer = {0};
	char              *url         = NULL;
	char              *data        = NULL;
	char              *e_owner     = NULL;
	char              *e_repo      = NULL;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	url  = sn_asprintf(
		"%s/repos/%s/%s/pulls/%d",
		gcli_get_apibase(),
		e_owner, e_repo, pr_number);
	data = sn_asprintf("{ \"state\": \"open\"}");

	gcli_fetch_with_method("PATCH", url, data, NULL, &json_buffer);

	free(json_buffer.data);
	free(url);
	free(data);
	free(e_owner);
	free(e_repo);
}

void
github_perform_submit_pull(gcli_submit_pull_options opts)
{
	sn_sv              e_head, e_base, e_title, e_body;
	gcli_fetch_buffer  fetch_buffer = {0};
	struct json_stream json         = {0};
	gcli_pull          pull         = {0};

	e_head  = gcli_json_escape(opts.from);
	e_base  = gcli_json_escape(opts.to);
	e_title = gcli_json_escape(opts.title);
	e_body  = gcli_json_escape(opts.body);

	char *post_fields = sn_asprintf(
		"{\"head\":\""SV_FMT"\",\"base\":\""SV_FMT"\", "
		"\"title\": \""SV_FMT"\", \"body\": \""SV_FMT"\" }",
		SV_ARGS(e_head),
		SV_ARGS(e_base),
		SV_ARGS(e_title),
		SV_ARGS(e_body));
	char *url         = sn_asprintf(
		"%s/repos/%s/%s/pulls",
		gcli_get_apibase(),
		opts.owner, opts.repo);

	gcli_fetch_with_method("POST", url, post_fields, NULL, &fetch_buffer);

	/* Add labels if requested. GitHub doesn't allow us to do this all
	 * with one request. */
	if (opts.labels_size) {
		json_open_buffer(&json, fetch_buffer.data, fetch_buffer.length);
		parse_github_pull(&json, &pull);

		github_issue_add_labels(opts.owner, opts.repo, pull.id,
		                        opts.labels, opts.labels_size);

		gcli_pull_free(&pull);
		json_close(&json);
	}

	free(fetch_buffer.data);
	free(e_head.data);
	free(e_base.data);
	free(e_title.data);
	free(e_body.data);
	free(post_fields);
	free(url);
}

int
github_get_pull_commits(char const *owner,
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

	url = sn_asprintf(
		"%s/repos/%s/%s/pulls/%d/commits",
		gcli_get_apibase(),
		e_owner, e_repo, pr_number);

	do {
		gcli_fetch(url, &next_url, &json_buffer);
		json_open_buffer(&stream, json_buffer.data, json_buffer.length);

		parse_github_commits(&stream, out, &count);

		json_close(&stream);
		free(json_buffer.data);
		free(url);
	} while ((url = next_url));

	free(e_owner);
	free(e_repo);

	return (int)count;
}

void
github_get_pull(char const *owner,
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

	url = sn_asprintf(
		"%s/repos/%s/%s/pulls/%d",
		gcli_get_apibase(),
		e_owner, e_repo, pr_number);
	gcli_fetch(url, NULL, &json_buffer);

	json_open_buffer(&stream, json_buffer.data, json_buffer.length);

	parse_github_pull(&stream, out);

	json_close(&stream);
	free(url);
	free(e_owner);
	free(e_repo);
	free(json_buffer.data);
}

int
github_pull_checks(char const *owner, char const *repo, int const pr_number)
{
	char refname[64] = {0};

	/* This is kind of a hack, but it works! */
	snprintf(refname, sizeof refname, "refs%%2Fpull%%2F%d%%2Fhead", pr_number);

	return github_checks(owner, repo, refname, -1);
}
