#include <templates/github/issues.h>
#include <templates/github/pulls.h>
#include <templates/github/labels.h>

#include <gcli/ctx.h>

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

static gcli_forge_type
get_github_forge_type(gcli_ctx *ctx)
{
	(void) ctx;
	return GCLI_FORGE_GITHUB;
}

static void
issues(struct json_stream *stream)
{
	gcli_issue issue = {0};
	gcli_ctx *ctx;

	assert(gcli_init(&ctx, get_github_forge_type, NULL, NULL) == NULL);
	parse_github_issue(ctx, stream, &issue);

	printf("title\t"SV_FMT"\n", SV_ARGS(issue.title));
	printf("number\t%d\n", issue.number);
	printf("author\t"SV_FMT"\n", SV_ARGS(issue.author));
	printf("locked\t%d\n", issue.locked);
	printf("state\t"SV_FMT"\n", SV_ARGS(issue.state));
	printf("labels_size\t%zu\n", issue.labels_size);
	printf("comments\t%d\n", issue.comments);
}

static void
pulls(struct json_stream *stream)
{
	gcli_pull pull = {0};
	gcli_ctx *ctx;

	assert(gcli_init(&ctx, get_github_forge_type, NULL, NULL) == NULL);
	parse_github_pull(ctx, stream, &pull);

	printf("title\t%s\n", pull.title);
	printf("state\t%s\n", pull.state);
	printf("author\t%s\n", pull.author);
	printf("number\t%d\n", pull.number);
	printf("id\t%d\n", pull.id);
	printf("merged\t%d\n", pull.merged);
}

static void
labels(struct json_stream *stream)
{
	gcli_label label = {0};
	gcli_ctx *ctx;

	assert(gcli_init(&ctx, get_github_forge_type, NULL, NULL) == NULL);
	parse_github_label(ctx, stream, &label);

	printf("id\t%ld\n", label.id);
	printf("name\t%s\n", label.name);
	printf("description\t%s\n", label.description);
	printf("colour\t%"PRIx32"\n", label.colour);
}

int
main(int argc, char *argv[])
{
	struct json_stream str = {0};

	(void) argc;

	json_open_stream(&str, stdin);
	json_set_streaming(&str, 1);

	if (strcmp(argv[1], "issues") == 0)
		issues(&str);
	else if (strcmp(argv[1], "pulls") == 0)
		pulls(&str);
	else if (strcmp(argv[1], "labels") == 0)
		labels(&str);
	else
		fprintf(stderr, "error: unknown subcommand\n");

	return 0;
}
