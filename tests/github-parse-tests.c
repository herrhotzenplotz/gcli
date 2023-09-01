#include <templates/github/issues.h>
#include <templates/github/pulls.h>
#include <templates/github/labels.h>

#include <err.h>
#include <string.h>

#include <gcli/ctx.h>

#include <atf-c.h>

#include <config.h>

static gcli_forge_type
get_github_forge_type(gcli_ctx *ctx)
{
	(void) ctx;
	return GCLI_FORGE_GITHUB;
}

static gcli_ctx *
test_context(void)
{
	gcli_ctx *ctx;
	ATF_REQUIRE(gcli_init(&ctx, get_github_forge_type, NULL, NULL) == NULL);
	return ctx;
}

static FILE *
open_sample(char const *const name)
{
	FILE *r;
	char p[4096] = {0};

	snprintf(p, sizeof p, "%s/samples/%s", TESTSRCDIR, name);

	r = fopen(p, "r");
	return r;
}

ATF_TC_WITHOUT_HEAD(simple_github_issue);
ATF_TC_BODY(simple_github_issue, tc)
{
	gcli_issue issue = {0};
	FILE *f;
	json_stream stream;
	gcli_ctx *ctx = test_context();

	ATF_REQUIRE(f = open_sample("github_simple_issue.json"));
	json_open_stream(&stream, f);

	ATF_REQUIRE(parse_github_issue(ctx, &stream, &issue) == 0);

	ATF_CHECK(issue.number = 115);
	ATF_CHECK(sn_sv_eq_to(issue.title, "consider removing FILE *out from printing functions"));
	ATF_CHECK(sn_sv_eq_to(issue.created_at, "2022-03-22T16:06:10Z"));
	ATF_CHECK(sn_sv_eq_to(issue.author, "herrhotzenplotz"));
	ATF_CHECK(sn_sv_eq_to(issue.state, "closed"));
	ATF_CHECK(issue.comments == 0);
	ATF_CHECK(issue.locked == false);

	ATF_CHECK(sn_sv_eq_to(
		          issue.body,
		          "We use these functions with ghcli only anyways. In "
		          "the GUI stuff we use the datastructures returned by "
		          "the api directly. And If we output, it is stdout "
		          "everywhere.\n"));

	ATF_CHECK(issue.labels_size == 0);
	ATF_CHECK(issue.labels == NULL);

	ATF_CHECK(issue.assignees_size == 0);
	ATF_CHECK(issue.assignees == NULL);

	ATF_CHECK(issue.is_pr == 0);
	ATF_CHECK(sn_sv_null(issue.milestone));
}

ATF_TC_WITHOUT_HEAD(simple_github_pull);
ATF_TC_BODY(simple_github_pull, tc)
{
	gcli_pull pull = {0};
	FILE *f;
	json_stream stream;
	gcli_ctx *ctx = test_context();

	ATF_REQUIRE(f = open_sample("github_simple_pull.json"));
	json_open_stream(&stream, f);

	ATF_REQUIRE(parse_github_pull(ctx, &stream, &pull) == 0);

	ATF_CHECK_STREQ(pull.author, "herrhotzenplotz");
	ATF_CHECK_STREQ(pull.state, "closed");
	ATF_CHECK_STREQ(pull.title, "mark notifications as read/done");
	ATF_CHECK_STREQ(pull.body, "Fixes #99\n");
	ATF_CHECK_STREQ(pull.created_at, "2022-03-22T13:20:57Z");
	ATF_CHECK_STREQ(pull.head_label, "herrhotzenplotz:99");
	ATF_CHECK_STREQ(pull.base_label, "herrhotzenplotz:trunk");
	ATF_CHECK_STREQ(pull.head_sha, "a00f475af1e31d56c7a5839508a21e2b76a31e49");
	ATF_CHECK(pull.milestone == NULL);
	ATF_CHECK(pull.id == 886044243);
	ATF_CHECK(pull.comments == 0);
	ATF_CHECK(pull.additions == 177);
	ATF_CHECK(pull.deletions == 82);
	ATF_CHECK(pull.commits == 6);
	ATF_CHECK(pull.changed_files == 13);

	ATF_CHECK(pull.labels == NULL);
	ATF_CHECK(pull.labels_size == 0);
	ATF_CHECK(pull.merged == true);
	ATF_CHECK(pull.mergeable == false);
	ATF_CHECK(pull.draft == false);
}

ATF_TC_WITHOUT_HEAD(simple_github_label);
ATF_TC_BODY(simple_github_label, tc)
{
	gcli_label label = {0};
	FILE *f;
	json_stream stream;
	gcli_ctx *ctx = test_context();

	ATF_REQUIRE(f = open_sample("github_simple_label.json"));
	json_open_stream(&stream, f);

	ATF_REQUIRE(parse_github_label(ctx, &stream, &label) == 0);

	ATF_CHECK(label.id == 208045946);
	ATF_CHECK_STREQ(label.name, "bug");
	ATF_CHECK_STREQ(label.description, "Something isn't working");
	ATF_CHECK(label.colour == 0xf2951300);
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, simple_github_issue);
	ATF_TP_ADD_TC(tp, simple_github_pull);
	ATF_TP_ADD_TC(tp, simple_github_label);

	return atf_no_error();
}
