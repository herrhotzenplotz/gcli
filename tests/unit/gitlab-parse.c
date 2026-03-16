/*
 * Copyright Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <templates/gitlab/forks.h>
#include <templates/gitlab/issues.h>
#include <templates/gitlab/labels.h>
#include <templates/gitlab/merge_requests.h>
#include <templates/gitlab/milestones.h>
#include <templates/gitlab/pipelines.h>
#include <templates/gitlab/releases.h>
#include <templates/gitlab/repos.h>
#include <templates/gitlab/snippets.h>

#include <string.h>

#include <gcli/ctx.h>
#include <gcli/gitlab/api.h>
#include <gcli/port/util.h>

#include "unit.h"

static gcli_forge_type
get_gitlab_forge_type(struct gcli_ctx *ctx)
{
	(void) ctx;
	return GCLI_FORGE_GITLAB;
}

static struct gcli_ctx *
test_context(UNIT_CTX)
{
	struct gcli_ctx *ctx;
	REQUIRE(gcli_init(&ctx, get_gitlab_forge_type, NULL, NULL) == NULL);
	return ctx;
}

static FILE *
open_sample(UNIT_CTX, char const *const name)
{
	FILE *r;
	char p[4096] = {0};

	snprintf(p, sizeof p, "%s/unit/samples/%s", TESTSRCDIR, name);

	REQUIRE((r = fopen(p, "r")) != NULL);

	return r;
}

DEFINE_TESTCASE(gitlab_simple_merge_request)
{
	struct json_stream stream = {0};
	struct gcli_ctx *ctx = test_context(UNIT_CTX_VAR);
	FILE *f = open_sample(UNIT_CTX_VAR, "gitlab_simple_merge_request.json");
	struct gcli_pull pull = {0};

	json_open_stream(&stream, f);
	REQUIRE(parse_gitlab_mr(ctx, &stream, &pull) == 0);

	CHECK_STREQ(pull.author, "herrhotzenplotz");
	CHECK_STREQ(pull.state, "merged");
	CHECK_STREQ(pull.title, "Fix test suite");
	CHECK_STREQ(pull.body, "This finally fixes the broken test suite");
	CHECK_EQ(pull.created_at, 1693525070);
	CHECK(pull.commits_link == NULL);
	CHECK_STREQ(pull.head_label, "fix-test-suite");
	CHECK_STREQ(pull.base_label, "trunk");
	CHECK_STREQ(pull.head_sha, "3eab596a6806434e4a34bb19de12307ab1217af3");
	CHECK(pull.milestone == NULL);
	CHECK(pull.id == 246912053);
	CHECK(pull.number == 216);
	CHECK(pull.comments == 0); // not supported
	CHECK(pull.additions == 0); // not supported
	CHECK(pull.deletions == 0); // not supported
	CHECK(pull.commits == 0); // not supported
	CHECK(pull.changed_files == 0); // not supported
	CHECK(pull.head_pipeline_id == 989409992);
	CHECK(pull.labels_size == 0);
	CHECK(pull.labels == NULL);
	CHECK(pull.merged == false);
	CHECK(pull.mergeable == true);
	CHECK(pull.draft == false);

	json_close(&stream);
	gcli_pull_free(&pull);
	gcli_destroy(&ctx);
}

DEFINE_TESTCASE(gitlab_simple_issue)
{
	struct json_stream stream = {0};
	struct gcli_ctx *ctx = test_context(UNIT_CTX_VAR);
	FILE *f = open_sample(UNIT_CTX_VAR, "gitlab_simple_issue.json");
	struct gcli_issue issue = {0};

	json_open_stream(&stream, f);
	REQUIRE(parse_gitlab_issue(ctx, &stream, &issue) == 0);

	CHECK(issue.number == 193);
	CHECK_STREQ(issue.title, "Make notifications API use a list struct containing both the ptr and size");
	CHECK_EQ(issue.created_at, 1691952185);
	CHECK_STREQ(issue.author, "herrhotzenplotz");
	CHECK_STREQ(issue.state, "closed");
	CHECK(issue.comments == 2);
	CHECK(issue.locked == false);
	CHECK_STREQ(issue.body, "That would make some of the code much cleaner");
	CHECK(issue.labels_size == 1);
	CHECK_STREQ(issue.labels[0], "good-first-issue");
	CHECK(issue.assignees == NULL);
	CHECK(issue.assignees_size == 0);
	CHECK(issue.is_pr == 0);
	CHECK(issue.milestone == NULL);

	json_close(&stream);
	gcli_issue_free(&issue);
	gcli_destroy(&ctx);
}

DEFINE_TESTCASE(gitlab_simple_label)
{
	struct json_stream stream = {0};
	struct gcli_ctx *ctx = test_context(UNIT_CTX_VAR);
	FILE *f = open_sample(UNIT_CTX_VAR, "gitlab_simple_label.json");
	struct gcli_label label = {0};

	json_open_stream(&stream, f);
	REQUIRE(parse_gitlab_label(ctx, &stream, &label) == 0);

	CHECK(label.id == 24376073);
	CHECK_STREQ(label.name, "bug");
	CHECK_STREQ(label.description, "Something isn't working as expected");
	CHECK(label.colour == 0xD73A4A00);

	json_close(&stream);
	gcli_free_label(&label);
	gcli_destroy(&ctx);
}

DEFINE_TESTCASE(gitlab_simple_release)
{
	struct json_stream stream = {0};
	struct gcli_ctx *ctx = test_context(UNIT_CTX_VAR);
	FILE *f = open_sample(UNIT_CTX_VAR, "gitlab_simple_release.json");
	struct gcli_release release = {0};

	json_open_stream(&stream, f);
	REQUIRE(parse_gitlab_release(ctx, &stream, &release) == 0);

	/* NOTE(Nico): this silly hack is needed as the fixup is only
	 * applied internally when you fetch the release list using the
	 * public library API. */
	gitlab_fixup_release_assets(ctx, &release);

	/* NOTE(Nico): on gitlab this is the tag name */
	CHECK_STREQ(release.id, "1.2.0");
	CHECK(release.assets_size == 4);
	{
		CHECK_STREQ(release.assets[0].name, "gcli-1.2.0.zip");
		CHECK_STREQ(release.assets[0].url,
		            "https://gitlab.com/herrhotzenplotz/gcli/-/archive/1.2.0/gcli-1.2.0.zip");
	}
	{
		CHECK_STREQ(release.assets[1].name, "gcli-1.2.0.tar.gz");
		CHECK_STREQ(release.assets[1].url,
		            "https://gitlab.com/herrhotzenplotz/gcli/-/archive/1.2.0/gcli-1.2.0.tar.gz");
	}
	{
		CHECK_STREQ(release.assets[2].name, "gcli-1.2.0.tar.bz2");
		CHECK_STREQ(release.assets[2].url,
		            "https://gitlab.com/herrhotzenplotz/gcli/-/archive/1.2.0/gcli-1.2.0.tar.bz2");
	}
	{
		CHECK_STREQ(release.assets[3].name, "gcli-1.2.0.tar");
		CHECK_STREQ(release.assets[3].url,
		            "https://gitlab.com/herrhotzenplotz/gcli/-/archive/1.2.0/gcli-1.2.0.tar");
	}

	CHECK_STREQ(release.name, "1.2.0");
	CHECK_STREQ(release.body, "# Version 1.2.0\n\nThis is version 1.2.0 of gcli.\n\n## Notes\n\nPlease test and report bugs.\n\nYou can download autotoolized tarballs at: https://herrhotzenplotz.de/gcli/releases/gcli-1.2.0/\n\n## Bug Fixes\n\n- Fix compile error when providing --with-libcurl without any arguments\n- Fix memory leaks in string processing functions\n- Fix missing nul termination in read-file function\n- Fix segmentation fault when clearing the milestone of a PR on Gitea\n- Fix missing documentation for milestone action in issues and pulls\n- Set the 'merged' flag properly when showing Gitlab merge requests\n\n## New features\n\n- Add a config subcommand for managing ssh keys (see gcli-config(1))\n- Show number of comments/notes in list of issues and PRs\n- Add support for milestone management in pull requests\n");
	CHECK_STREQ(release.author, "herrhotzenplotz");
	CHECK_EQ(release.date, 1691740566);
	CHECK(release.upload_url == NULL);
	CHECK(release.draft == false);
	CHECK(release.prerelease == false);

	json_close(&stream);
	gcli_release_free(&release);
	gcli_destroy(&ctx);
}

DEFINE_TESTCASE(gitlab_simple_fork)
{
	struct json_stream stream = {0};
	struct gcli_ctx *ctx = test_context(UNIT_CTX_VAR);
	FILE *f = open_sample(UNIT_CTX_VAR, "gitlab_simple_fork.json");
	struct gcli_fork fork = {0};

	json_open_stream(&stream, f);
	REQUIRE(parse_gitlab_fork(ctx, &stream, &fork) == 0);

	CHECK_STREQ(fork.full_name, "gjnoonan/gcli");
	CHECK_STREQ(fork.owner, "gjnoonan");
	CHECK_EQ(fork.date, 1664718860);
	CHECK(fork.forks == 0);

	json_close(&stream);
	gcli_fork_free(&fork);
	gcli_destroy(&ctx);
}

DEFINE_TESTCASE(gitlab_simple_milestone)
{
	struct json_stream stream = {0};
	struct gcli_ctx *ctx = test_context(UNIT_CTX_VAR);
	FILE *f = open_sample(UNIT_CTX_VAR, "gitlab_simple_milestone.json");
	struct gcli_milestone milestone = {0};

	json_open_stream(&stream, f);
	REQUIRE(parse_gitlab_milestone(ctx, &stream, &milestone) == 0);

	CHECK(milestone.id == 2975318);
	CHECK_STREQ(milestone.title, "Version 2");
	CHECK_STREQ(milestone.state, "active");
	CHECK_STREQ(milestone.description,
	                "Things that need to be done for version 2");
	CHECK_EQ(milestone.created_at, 1675624100);
	CHECK_EQ(milestone.due_date, 0);
	CHECK_EQ(milestone.updated_at, 1675624100);
	CHECK(milestone.expired == false);

	json_close(&stream);
	gcli_free_milestone(&milestone);
	gcli_destroy(&ctx);

	/* Ignore open issues and closed issues as they are
	 * github/gitea-specific */
}

DEFINE_TESTCASE(gitlab_simple_pipeline)
{
	struct json_stream stream = {0};
	struct gcli_ctx *ctx = test_context(UNIT_CTX_VAR);
	FILE *f = open_sample(UNIT_CTX_VAR, "gitlab_simple_pipeline.json");
	struct gitlab_pipeline pipeline = {0};

	json_open_stream(&stream, f);
	REQUIRE(parse_gitlab_pipeline(ctx, &stream, &pipeline) == 0);

	CHECK(pipeline.id == 989897020);
	CHECK_STREQ(pipeline.status, "failed");
	CHECK_EQ(pipeline.created_at, 1693665020);
	CHECK_EQ(pipeline.updated_at, 1693665100);
	CHECK_STREQ(pipeline.ref, "refs/merge-requests/219/head");
	CHECK_STREQ(pipeline.sha, "742affb88a297a6b34201ad61c8b5b72ec6eb679");
	CHECK_STREQ(pipeline.source, "merge_request_event");

	json_close(&stream);
	gitlab_pipeline_free(&pipeline);
	gcli_destroy(&ctx);
}

DEFINE_TESTCASE(gitlab_simple_repo)
{
	struct json_stream stream = {0};
	struct gcli_ctx *ctx = test_context(UNIT_CTX_VAR);
	FILE *f = open_sample(UNIT_CTX_VAR, "gitlab_simple_repo.json");
	struct gcli_repo repo = {0};

	json_open_stream(&stream, f);
	REQUIRE(parse_gitlab_repo(ctx, &stream, &repo) == 0);

	CHECK(repo.id == 34707535);
	CHECK_STREQ(repo.full_name, "herrhotzenplotz/gcli");
	CHECK_STREQ(repo.name, "gcli");
	CHECK_STREQ(repo.owner, "herrhotzenplotz");
	CHECK_EQ(repo.date, 1647968279);
	CHECK_STREQ(repo.visibility, "public");
	CHECK(repo.is_fork == false);

	json_close(&stream);
	gcli_repo_free(&repo);
	gcli_destroy(&ctx);
}

DEFINE_TESTCASE(gitlab_simple_snippet)
{
	struct json_stream stream = {0};
	struct gcli_ctx *ctx = test_context(UNIT_CTX_VAR);
	FILE *f = open_sample(UNIT_CTX_VAR, "gitlab_simple_snippet.json");
	struct gcli_gitlab_snippet snippet = {0};

	json_open_stream(&stream, f);
	REQUIRE(parse_gitlab_snippet(ctx, &stream, &snippet) == 0);

	CHECK(snippet.id == 2141655);
	CHECK_STREQ(snippet.title, "darcy-weisbach SPARC64");
	CHECK_STREQ(snippet.filename, "darcy-weisbach SPARC64");
	CHECK_STREQ(snippet.date, "2021-06-28T15:47:36.214Z");
	CHECK_STREQ(snippet.author, "herrhotzenplotz");
	CHECK_STREQ(snippet.visibility, "public");
	CHECK_STREQ(snippet.raw_url, "https://gitlab.com/-/snippets/2141655/raw");

	json_close(&stream);
	gcli_gitlab_snippet_free(&snippet);
	gcli_destroy(&ctx);
}

DEFINE_TESTCASE(gitlab_error_token_expired)
{
	struct gcli_ctx *ctx = test_context(UNIT_CTX_VAR);
	struct gcli_fetch_buffer buffer = {0};
	char const *errmsg = NULL;

	buffer.length = gcli_read_file(TESTSRCDIR"unit/samples/gitlab_token_expired.json",
	                               &buffer.data);

	REQUIRE(buffer.length);
	errmsg = gitlab_api_error_string(ctx, &buffer);

	CHECK_STREQ(errmsg, "Token has expired.");
}

DEFINE_TESTCASE(gitlab_error_unauthorised)
{
	struct gcli_ctx *ctx = test_context(UNIT_CTX_VAR);
	struct gcli_fetch_buffer buffer = {0};
	char const *errmsg = NULL;

	buffer.length = gcli_read_file(TESTSRCDIR"/unit/samples/gitlab_error_unauthorised.json",
	                               &buffer.data);

	REQUIRE(buffer.length);
	errmsg = gitlab_api_error_string(ctx, &buffer);

	CHECK_STREQ(errmsg, "401 Unauthorized");
}

TESTSUITE
{
	TESTCASE(gitlab_simple_fork);
	TESTCASE(gitlab_simple_issue);
	TESTCASE(gitlab_simple_label);
	TESTCASE(gitlab_simple_merge_request);
	TESTCASE(gitlab_simple_milestone);
	TESTCASE(gitlab_simple_pipeline);
	TESTCASE(gitlab_simple_release);
	TESTCASE(gitlab_simple_repo);
	TESTCASE(gitlab_simple_snippet);
	TESTCASE(gitlab_error_token_expired);
	TESTCASE(gitlab_error_unauthorised);
}
