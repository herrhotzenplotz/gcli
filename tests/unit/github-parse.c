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

#include <templates/github/checks.h>
#include <templates/github/comments.h>
#include <templates/github/forks.h>
#include <templates/github/issues.h>
#include <templates/github/labels.h>
#include <templates/github/milestones.h>
#include <templates/github/pulls.h>
#include <templates/github/releases.h>
#include <templates/github/repos.h>

#include <string.h>

#include <gcli/ctx.h>

#include "unit.h"

static gcli_forge_type
get_github_forge_type(struct gcli_ctx *ctx)
{
	(void) ctx;
	return GCLI_FORGE_GITHUB;
}

static struct gcli_ctx *
test_context(UNIT_CTX)
{
	struct gcli_ctx *ctx;
	REQUIRE(gcli_init(&ctx, get_github_forge_type, NULL, NULL) == NULL);
	return ctx;
}

static FILE *
open_sample(char const *const name)
{
	FILE *r;
	char p[4096] = {0};

	snprintf(p, sizeof p, "%s/unit/samples/%s", TESTSRCDIR, name);

	r = fopen(p, "r");
	return r;
}

DEFINE_TESTCASE(simple_github_issue)
{
	struct gcli_issue issue = {0};
	FILE *f;
	struct json_stream stream;
	struct gcli_ctx *ctx = test_context(UNIT_CTX_VAR);

	REQUIRE((f = open_sample("github_simple_issue.json")) != NULL);
	json_open_stream(&stream, f);

	REQUIRE(parse_github_issue(ctx, &stream, &issue) == 0);

	CHECK_EQ(issue.number, 115);
	CHECK_STREQ(issue.title, "consider removing FILE *out from printing functions");
	CHECK_EQ(issue.created_at, 1647965170);
	CHECK_STREQ(issue.author, "herrhotzenplotz");
	CHECK_STREQ(issue.state, "closed");
	CHECK(issue.comments == 0);
	CHECK(issue.locked == false);

	CHECK_STREQ(issue.body,
		        "We use these functions with ghcli only anyways. In "
		        "the GUI stuff we use the datastructures returned by "
		        "the api directly. And If we output, it is stdout "
		        "everywhere.\n");

	CHECK(issue.labels_size == 0);
	CHECK(issue.labels == NULL);

	CHECK(issue.assignees_size == 0);
	CHECK(issue.assignees == NULL);

	CHECK(issue.is_pr == 0);
	CHECK(!issue.milestone);

	json_close(&stream);
	gcli_issue_free(&issue);
	gcli_destroy(&ctx);
}

DEFINE_TESTCASE(simple_github_pull)
{
	struct gcli_pull pull = {0};
	FILE *f;
	struct json_stream stream;
	struct gcli_ctx *ctx = test_context(UNIT_CTX_VAR);

	REQUIRE((f = open_sample("github_simple_pull.json")) != NULL);
	json_open_stream(&stream, f);

	REQUIRE(parse_github_pull(ctx, &stream, &pull) == 0);

	CHECK_STREQ(pull.author, "herrhotzenplotz");
	CHECK_STREQ(pull.state, "closed");
	CHECK_STREQ(pull.title, "mark notifications as read/done");
	CHECK_STREQ(pull.body, "Fixes #99\n");
	CHECK_EQ(pull.created_at, 1647955257);
	CHECK_STREQ(pull.head_label, "herrhotzenplotz:99");
	CHECK_STREQ(pull.base_label, "herrhotzenplotz:trunk");
	CHECK_STREQ(pull.head_sha, "a00f475af1e31d56c7a5839508a21e2b76a31e49");
	CHECK(pull.milestone == NULL);
	CHECK(pull.id == 886044243);
	CHECK(pull.comments == 0);
	CHECK(pull.additions == 177);
	CHECK(pull.deletions == 82);
	CHECK(pull.commits == 6);
	CHECK(pull.changed_files == 13);

	CHECK(pull.labels == NULL);
	CHECK(pull.labels_size == 0);
	CHECK(pull.merged == true);
	CHECK(pull.mergeable == false);
	CHECK(pull.draft == false);

	json_close(&stream);
	gcli_pull_free(&pull);
	gcli_destroy(&ctx);
}

DEFINE_TESTCASE(simple_github_label)
{
	struct gcli_label label = {0};
	FILE *f;
	struct json_stream stream;
	struct gcli_ctx *ctx = test_context(UNIT_CTX_VAR);

	REQUIRE((f = open_sample("github_simple_label.json")) != NULL);
	json_open_stream(&stream, f);

	REQUIRE(parse_github_label(ctx, &stream, &label) == 0);

	CHECK(label.id == 3431203676);
	CHECK_STREQ(label.name, "bug");
	CHECK_STREQ(label.description, "Something isn't working");
	CHECK(label.colour == 0xd73a4a00);

	json_close(&stream);
	gcli_free_label(&label);
	gcli_destroy(&ctx);
}

DEFINE_TESTCASE(simple_github_milestone)
{
	struct gcli_milestone milestone = {0};
	FILE *f;
	struct json_stream stream;
	struct gcli_ctx *ctx = test_context(UNIT_CTX_VAR);

	REQUIRE((f = open_sample("github_simple_milestone.json")) != NULL);
	json_open_stream(&stream, f);

	REQUIRE(parse_github_milestone(ctx, &stream, &milestone) == 0);

	CHECK(milestone.id == 1);
	CHECK_STREQ(milestone.title, "Gitlab support");
	CHECK_STREQ(milestone.state, "open");
	CHECK_EQ(milestone.created_at, 1639465325);
	CHECK_STREQ(milestone.description, "");
	CHECK_EQ(milestone.updated_at, 1639925383);
	CHECK_EQ(milestone.due_date, 0);
	CHECK(milestone.expired == false);
	CHECK(milestone.open_issues == 0);
	CHECK(milestone.closed_issues == 8);

	json_close(&stream);
	gcli_free_milestone(&milestone);
	gcli_destroy(&ctx);
}

DEFINE_TESTCASE(simple_github_release)
{
	struct gcli_release release = {0};
	FILE *f;
	struct json_stream stream;
	struct gcli_ctx *ctx = test_context(UNIT_CTX_VAR);

	REQUIRE((f = open_sample("github_simple_release.json")) != NULL);
	json_open_stream(&stream, f);

	REQUIRE(parse_github_release(ctx, &stream, &release) == 0);

	CHECK_STREQ(release.id, "116031718");
	CHECK(release.assets_size == 0);
	CHECK(release.assets == NULL);
	CHECK_STREQ(release.name, "1.2.0");
	CHECK_STREQ(release.body, "# Version 1.2.0\n\nThis is version 1.2.0 of gcli.\n\n## Notes\n\nPlease test and report bugs.\n\nYou can download autotoolized tarballs at: https://herrhotzenplotz.de/gcli/releases/gcli-1.2.0/\n\n## Bug Fixes\n\n- Fix compile error when providing --with-libcurl without any arguments\n- Fix memory leaks in string processing functions\n- Fix missing nul termination in read-file function\n- Fix segmentation fault when clearing the milestone of a PR on Gitea\n- Fix missing documentation for milestone action in issues and pulls\n- Set the 'merged' flag properly when showing Gitlab merge requests\n\n## New features\n\n- Add a config subcommand for managing ssh keys (see gcli-config(1))\n- Show number of comments/notes in list of issues and PRs\n- Add support for milestone management in pull requests\n");
	CHECK_STREQ(release.author, "herrhotzenplotz");
	CHECK_EQ(release.date, 1691739757);
	CHECK_STREQ(release.upload_url, "https://uploads.github.com/repos/herrhotzenplotz/gcli/releases/116031718/assets{?name,label}");
	CHECK(release.draft == false);
	CHECK(release.prerelease == false);

	json_close(&stream);
	gcli_release_free(&release);
	gcli_destroy(&ctx);
}

DEFINE_TESTCASE(simple_github_repo)
{
	struct gcli_repo repo = {0};
	FILE *f;
	struct json_stream stream;
	struct gcli_ctx *ctx = test_context(UNIT_CTX_VAR);

	REQUIRE((f = open_sample("github_simple_repo.json")) != NULL);
	json_open_stream(&stream, f);

	REQUIRE(parse_github_repo(ctx, &stream, &repo) == 0);

	CHECK(repo.id == 415015197);
	CHECK_STREQ(repo.full_name, "herrhotzenplotz/gcli");
	CHECK_STREQ(repo.name, "gcli");
	CHECK_STREQ(repo.owner, "herrhotzenplotz");
	CHECK_EQ(repo.date, 1633702815);
	CHECK_STREQ(repo.visibility, "public");
	CHECK(repo.is_fork == false);

	json_close(&stream);
	gcli_repo_free(&repo);
	gcli_destroy(&ctx);
}

DEFINE_TESTCASE(simple_github_fork)
{
	struct gcli_fork fork = {0};
	FILE *f;
	struct json_stream stream;
	struct gcli_ctx *ctx = test_context(UNIT_CTX_VAR);

	REQUIRE((f = open_sample("github_simple_fork.json")) != NULL);
	json_open_stream(&stream, f);

	REQUIRE(parse_github_fork(ctx, &stream, &fork) == 0);

	CHECK_STREQ(fork.full_name, "gjnoonan/quick-lint-js");
	CHECK_STREQ(fork.owner, "gjnoonan");
	CHECK_EQ(fork.date, 1683783461);
	CHECK(fork.forks == 0);

	json_close(&stream);
	gcli_fork_free(&fork);
	gcli_destroy(&ctx);
}

DEFINE_TESTCASE(simple_github_comment)
{
	struct gcli_comment comment = {0};
	FILE *f;
	struct json_stream stream;
	struct gcli_ctx *ctx = test_context(UNIT_CTX_VAR);

	REQUIRE((f = open_sample("github_simple_comment.json")) != NULL);
	json_open_stream(&stream, f);

	REQUIRE(parse_github_comment(ctx, &stream, &comment) == 0);

	CHECK(comment.id == 1424392601);
	CHECK_STREQ(comment.author, "herrhotzenplotz");
	CHECK_EQ(comment.date, 1675957074);
	CHECK_STREQ(comment.body, "Hey,\n\nthe current trunk on Github might be a little outdated. I pushed the staging branch for version 1.0.0 from Gitlab to Github (cleanup-1.0). Could you try again with that branch and see if it still faults at the same place? If it does, please provide a full backtrace and if possible check with valgrind.\n");

	json_close(&stream);
	gcli_comment_free(&comment);
	gcli_destroy(&ctx);
}

DEFINE_TESTCASE(simple_github_check)
{
	struct gcli_github_check check = {0};
	FILE *f;
	struct json_stream stream;
	struct gcli_ctx *ctx = test_context(UNIT_CTX_VAR);

	REQUIRE((f = open_sample("github_simple_check.json")) != NULL);
	json_open_stream(&stream, f);

	REQUIRE(parse_github_check(ctx, &stream, &check) == 0);

	CHECK_STREQ(check.name, "test Windows x86");
	CHECK_STREQ(check.status, "completed");
	CHECK_STREQ(check.conclusion, "success");
	CHECK_STREQ(check.started_at, "2023-09-02T06:27:37Z");
	CHECK_STREQ(check.completed_at, "2023-09-02T06:29:11Z");
	CHECK(check.id == 16437184455);

	json_close(&stream);
	gcli_github_check_free(&check);
	gcli_destroy(&ctx);
}

TESTSUITE
{
	TESTCASE(simple_github_issue);
	TESTCASE(simple_github_pull);
	TESTCASE(simple_github_label);
	TESTCASE(simple_github_milestone);
	TESTCASE(simple_github_release);
	TESTCASE(simple_github_repo);
	TESTCASE(simple_github_fork);
	TESTCASE(simple_github_comment);
	TESTCASE(simple_github_check);
}
