#include <templates/gitlab/issues.h>
#include <templates/gitlab/merge_requests.h>

#include <err.h>
#include <string.h>

#include <gcli/ctx.h>

#include <atf-c.h>

#include <config.h>

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
	ATF_CHECK(sn_sv_eq_to(issue.title, "Make notifications API use a list struct containing both the ptr and size"));
	ATF_CHECK(sn_sv_eq_to(issue.created_at, "2023-08-13T18:43:05.766Z"));
	ATF_CHECK(sn_sv_eq_to(issue.author, "herrhotzenplotz"));
	ATF_CHECK(sn_sv_eq_to(issue.state, "closed"));
	ATF_CHECK(issue.comments == 2);
	ATF_CHECK(issue.locked == false);
	ATF_CHECK(sn_sv_eq_to(issue.body, "That would make some of the code much cleaner"));
	ATF_CHECK(issue.labels_size == 1);
	ATF_CHECK(sn_sv_eq_to(issue.labels[0], "good-first-issue"));
	ATF_CHECK(issue.assignees == NULL);
	ATF_CHECK(issue.assignees_size == 0);
	ATF_CHECK(issue.is_pr == 0);
	ATF_CHECK(sn_sv_null(issue.milestone));
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, gitlab_simple_merge_request);
	ATF_TP_ADD_TC(tp, gitlab_simple_issue);
	return atf_no_error();
}
