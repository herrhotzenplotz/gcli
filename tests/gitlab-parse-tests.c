#include <templates/gitlab/forks.h>
#include <templates/gitlab/issues.h>
#include <templates/gitlab/labels.h>
#include <templates/gitlab/merge_requests.h>
#include <templates/gitlab/milestones.h>
#include <templates/gitlab/pipelines.h>
#include <templates/gitlab/releases.h>
#include <templates/gitlab/repos.h>
#include <templates/gitlab/snippets.h>

#include <err.h>
#include <string.h>

#include <gcli/ctx.h>

#include "gcli_tests.h"

static gcli_forge_type
get_gitlab_forge_type(gcli_ctx *ctx)
{
	(void) ctx;
	return GCLI_FORGE_GITLAB;
}

static gcli_ctx *
test_context(void)
{
	gcli_ctx *ctx;
	ATF_REQUIRE(gcli_init(&ctx, get_gitlab_forge_type, NULL, NULL) == NULL);
	return ctx;
}

static FILE *
open_sample(char const *const name)
{
	FILE *r;
	char p[4096] = {0};

	snprintf(p, sizeof p, "%s/samples/%s", TESTSRCDIR, name);

	ATF_REQUIRE(r = fopen(p, "r"));

	return r;
}

ATF_TC_WITHOUT_HEAD(gitlab_simple_merge_request);
ATF_TC_BODY(gitlab_simple_merge_request, tc)
{
	json_stream stream = {0};
	gcli_ctx *ctx = test_context();
	FILE *f = open_sample("gitlab_simple_merge_request.json");
	gcli_pull pull = {0};

	json_open_stream(&stream, f);
	ATF_REQUIRE(parse_gitlab_mr(ctx, &stream, &pull) == 0);

	ATF_CHECK_STREQ(pull.author, "herrhotzenplotz");
	ATF_CHECK_STREQ(pull.state, "merged");
	ATF_CHECK_STREQ(pull.title, "Fix test suite");
	ATF_CHECK_STREQ(pull.body, "This finally fixes the broken test suite");
	ATF_CHECK_STREQ(pull.created_at, "2023-08-31T23:37:50.848Z");
	ATF_CHECK(pull.commits_link == NULL);
	ATF_CHECK_STREQ(pull.head_label, "fix-test-suite");
	ATF_CHECK_STREQ(pull.base_label, "trunk");
	ATF_CHECK_STREQ(pull.head_sha, "3eab596a6806434e4a34bb19de12307ab1217af3");
	ATF_CHECK(pull.milestone == NULL);
	ATF_CHECK(pull.id == 246912053);
	ATF_CHECK(pull.number == 216);
	ATF_CHECK(pull.comments == 0); // not supported
	ATF_CHECK(pull.additions == 0); // not supported
	ATF_CHECK(pull.deletions == 0); // not supported
	ATF_CHECK(pull.commits == 0); // not supported
	ATF_CHECK(pull.changed_files == 0); // not supported
	ATF_CHECK(pull.head_pipeline_id == 989409992);
	ATF_CHECK(pull.labels_size == 0);
	ATF_CHECK(pull.labels == NULL);
	ATF_CHECK(pull.merged == false);
	ATF_CHECK(pull.mergeable == true);
	ATF_CHECK(pull.draft == false);
}

ATF_TC_WITHOUT_HEAD(gitlab_simple_issue);
ATF_TC_BODY(gitlab_simple_issue, tc)
{
	json_stream stream = {0};
	gcli_ctx *ctx = test_context();
	FILE *f = open_sample("gitlab_simple_issue.json");
	gcli_issue issue = {0};

	json_open_stream(&stream, f);
	ATF_REQUIRE(parse_gitlab_issue(ctx, &stream, &issue) == 0);

	ATF_CHECK(issue.number == 193);
	ATF_CHECK_SV_EQTO(issue.title, "Make notifications API use a list struct containing both the ptr and size");
	ATF_CHECK_SV_EQTO(issue.created_at, "2023-08-13T18:43:05.766Z");
	ATF_CHECK_SV_EQTO(issue.author, "herrhotzenplotz");
	ATF_CHECK_SV_EQTO(issue.state, "closed");
	ATF_CHECK(issue.comments == 2);
	ATF_CHECK(issue.locked == false);
	ATF_CHECK_SV_EQTO(issue.body, "That would make some of the code much cleaner");
	ATF_CHECK(issue.labels_size == 1);
	ATF_CHECK_SV_EQTO(issue.labels[0], "good-first-issue");
	ATF_CHECK(issue.assignees == NULL);
	ATF_CHECK(issue.assignees_size == 0);
	ATF_CHECK(issue.is_pr == 0);
	ATF_CHECK(sn_sv_null(issue.milestone));
}

ATF_TC_WITHOUT_HEAD(gitlab_simple_label);
ATF_TC_BODY(gitlab_simple_label, tc)
{
	json_stream stream = {0};
	gcli_ctx *ctx = test_context();
	FILE *f = open_sample("gitlab_simple_label.json");
	gcli_label label = {0};

	json_open_stream(&stream, f);
	ATF_REQUIRE(parse_gitlab_label(ctx, &stream, &label) == 0);

	ATF_CHECK(label.id == 24376073);
	ATF_CHECK_STREQ(label.name, "bug");
	ATF_CHECK_STREQ(label.description, "Something isn't working as expected");
	ATF_CHECK(label.colour == 0xD73A4A00);
}

ATF_TC_WITHOUT_HEAD(gitlab_simple_release);
ATF_TC_BODY(gitlab_simple_release, tc)
{
	json_stream stream = {0};
	gcli_ctx *ctx = test_context();
	FILE *f = open_sample("gitlab_simple_release.json");
	gcli_release release = {0};

	json_open_stream(&stream, f);
	ATF_REQUIRE(parse_gitlab_release(ctx, &stream, &release) == 0);

	/* NOTE(Nico): this silly hack is needed as the fixup is only
	 * applied internally when you fetch the release list using the
	 * public library API. */
	gitlab_fixup_release_assets(ctx, &release);

	/* NOTE(Nico): on gitlab this is the tag name */
	ATF_CHECK_SV_EQTO(release.id, "1.2.0");
	ATF_CHECK(release.assets_size == 4);
	{
		ATF_CHECK_STREQ(release.assets[0].name, "gcli-1.2.0.zip");
		ATF_CHECK_STREQ(release.assets[0].url,
		                "https://gitlab.com/herrhotzenplotz/gcli/-/archive/1.2.0/gcli-1.2.0.zip");
	}
	{
		ATF_CHECK_STREQ(release.assets[1].name, "gcli-1.2.0.tar.gz");
		ATF_CHECK_STREQ(release.assets[1].url,
		                "https://gitlab.com/herrhotzenplotz/gcli/-/archive/1.2.0/gcli-1.2.0.tar.gz");
	}
	{
		ATF_CHECK_STREQ(release.assets[2].name, "gcli-1.2.0.tar.bz2");
		ATF_CHECK_STREQ(release.assets[2].url,
		                "https://gitlab.com/herrhotzenplotz/gcli/-/archive/1.2.0/gcli-1.2.0.tar.bz2");
	}
	{
		ATF_CHECK_STREQ(release.assets[3].name, "gcli-1.2.0.tar");
		ATF_CHECK_STREQ(release.assets[3].url,
		                "https://gitlab.com/herrhotzenplotz/gcli/-/archive/1.2.0/gcli-1.2.0.tar");
	}

	ATF_CHECK_SV_EQTO(release.name, "1.2.0");
	ATF_CHECK_SV_EQTO(release.body, "# Version 1.2.0\n\nThis is version 1.2.0 of gcli.\n\n## Notes\n\nPlease test and report bugs.\n\nYou can download autotoolized tarballs at: https://herrhotzenplotz.de/gcli/releases/gcli-1.2.0/\n\n## Bug Fixes\n\n- Fix compile error when providing --with-libcurl without any arguments\n- Fix memory leaks in string processing functions\n- Fix missing nul termination in read-file function\n- Fix segmentation fault when clearing the milestone of a PR on Gitea\n- Fix missing documentation for milestone action in issues and pulls\n- Set the 'merged' flag properly when showing Gitlab merge requests\n\n## New features\n\n- Add a config subcommand for managing ssh keys (see gcli-config(1))\n- Show number of comments/notes in list of issues and PRs\n- Add support for milestone management in pull requests\n");
	ATF_CHECK_SV_EQTO(release.author, "herrhotzenplotz");
	ATF_CHECK_SV_EQTO(release.date, "2023-08-11T07:56:06.371Z");
	ATF_CHECK(sn_sv_null(release.upload_url));
	ATF_CHECK(release.draft == false);
	ATF_CHECK(release.prerelease == false);
}

ATF_TC_WITHOUT_HEAD(gitlab_simple_fork);
ATF_TC_BODY(gitlab_simple_fork, tc)
{
	json_stream stream = {0};
	gcli_ctx *ctx = test_context();
	FILE *f = open_sample("gitlab_simple_fork.json");
	gcli_fork fork = {0};

	json_open_stream(&stream, f);
	ATF_REQUIRE(parse_gitlab_fork(ctx, &stream, &fork) == 0);

	ATF_CHECK_SV_EQTO(fork.full_name, "gjnoonan/gcli");
	ATF_CHECK_SV_EQTO(fork.owner, "gjnoonan");
	ATF_CHECK_SV_EQTO(fork.date, "2022-10-02T13:54:20.517Z");
	ATF_CHECK(fork.forks == 0);
}

ATF_TC_WITHOUT_HEAD(gitlab_simple_milestone);
ATF_TC_BODY(gitlab_simple_milestone, tc)
{
	json_stream stream = {0};
	gcli_ctx *ctx = test_context();
	FILE *f = open_sample("gitlab_simple_milestone.json");
	gcli_milestone milestone = {0};

	json_open_stream(&stream, f);
	ATF_REQUIRE(parse_gitlab_milestone(ctx, &stream, &milestone) == 0);

	ATF_CHECK(milestone.id == 2975318);
	ATF_CHECK_STREQ(milestone.title, "Version 2");
	ATF_CHECK_STREQ(milestone.state, "active");
	ATF_CHECK_STREQ(milestone.description,
	                "Things that need to be done for version 2");
	ATF_CHECK_STREQ(milestone.created_at, "2023-02-05T19:08:20.379Z");
	ATF_CHECK_STREQ(milestone.due_date, "<empty>");
	ATF_CHECK_STREQ(milestone.updated_at, "2023-02-05T19:08:20.379Z");
	ATF_CHECK(milestone.expired == false);

	/* Ignore open issues and closed issues as they are
	 * github/gitea-specific */
}

ATF_TC_WITHOUT_HEAD(gitlab_simple_pipeline);
ATF_TC_BODY(gitlab_simple_pipeline, tc)
{
	json_stream stream = {0};
	gcli_ctx *ctx = test_context();
	FILE *f = open_sample("gitlab_simple_pipeline.json");
	gitlab_pipeline pipeline = {0};

	json_open_stream(&stream, f);
	ATF_REQUIRE(parse_gitlab_pipeline(ctx, &stream, &pipeline) == 0);

	ATF_CHECK(pipeline.id == 989897020);
	ATF_CHECK_STREQ(pipeline.status, "failed");
	ATF_CHECK_STREQ(pipeline.created_at, "2023-09-02T14:30:20.925Z");
	ATF_CHECK_STREQ(pipeline.updated_at, "2023-09-02T14:31:40.328Z");
	ATF_CHECK_STREQ(pipeline.ref, "refs/merge-requests/219/head");
	ATF_CHECK_STREQ(pipeline.sha, "742affb88a297a6b34201ad61c8b5b72ec6eb679");
	ATF_CHECK_STREQ(pipeline.source, "merge_request_event");
}

ATF_TC_WITHOUT_HEAD(gitlab_simple_repo);
ATF_TC_BODY(gitlab_simple_repo, tc)
{
	json_stream stream = {0};
	gcli_ctx *ctx = test_context();
	FILE *f = open_sample("gitlab_simple_repo.json");
	gcli_repo repo = {0};

	json_open_stream(&stream, f);
	ATF_REQUIRE(parse_gitlab_repo(ctx, &stream, &repo) == 0);

	ATF_CHECK(repo.id == 34707535);
	ATF_CHECK_SV_EQTO(repo.full_name, "herrhotzenplotz/gcli");
	ATF_CHECK_SV_EQTO(repo.name, "gcli");
	ATF_CHECK_SV_EQTO(repo.owner, "herrhotzenplotz");
	ATF_CHECK_SV_EQTO(repo.date, "2022-03-22T16:57:59.891Z");
	ATF_CHECK_SV_EQTO(repo.visibility, "public");
	ATF_CHECK(repo.is_fork == false);
}

ATF_TC_WITHOUT_HEAD(gitlab_simple_snippet);
ATF_TC_BODY(gitlab_simple_snippet, tc)
{
	json_stream stream = {0};
	gcli_ctx *ctx = test_context();
	FILE *f = open_sample("gitlab_simple_snippet.json");
	gcli_gitlab_snippet snippet = {0};

	json_open_stream(&stream, f);
	ATF_REQUIRE(parse_gitlab_snippet(ctx, &stream, &snippet) == 0);

	ATF_CHECK(snippet.id == 2141655);
	ATF_CHECK_STREQ(snippet.title, "darcy-weisbach SPARC64");
	ATF_CHECK_STREQ(snippet.filename, "darcy-weisbach SPARC64");
	ATF_CHECK_STREQ(snippet.date, "2021-06-28T15:47:36.214Z");
	ATF_CHECK_STREQ(snippet.author, "herrhotzenplotz");
	ATF_CHECK_STREQ(snippet.visibility, "public");
	ATF_CHECK_STREQ(snippet.raw_url, "https://gitlab.com/-/snippets/2141655/raw");
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, gitlab_simple_fork);
	ATF_TP_ADD_TC(tp, gitlab_simple_issue);
	ATF_TP_ADD_TC(tp, gitlab_simple_label);
	ATF_TP_ADD_TC(tp, gitlab_simple_merge_request);
	ATF_TP_ADD_TC(tp, gitlab_simple_milestone);
	ATF_TP_ADD_TC(tp, gitlab_simple_pipeline);
	ATF_TP_ADD_TC(tp, gitlab_simple_release);
	ATF_TP_ADD_TC(tp, gitlab_simple_repo);
	ATF_TP_ADD_TC(tp, gitlab_simple_snippet);

	return atf_no_error();
}
