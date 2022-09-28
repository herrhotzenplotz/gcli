#include <stdio.h>
#include <stdlib.h>

#include <gcli/github/parsers/issues.h>

static void
issue(void)
{
    struct json_stream input = {0};
    gcli_issue issue = {0};

    json_open_stream(&input, stdin);
    github_parse_issue(&input, &issue);

    printf("title\t"SV_FMT"\n", SV_ARGS(issue.title));
    printf("number\t%d\n", issue.number);
    printf("author\t"SV_FMT"\n", SV_ARGS(issue.author));
    printf("locked\t%d\n", issue.locked);
    printf("state\t"SV_FMT"\n", SV_ARGS(issue.state));
    printf("labels_size\t%zu\n", issue.labels_size);
    printf("comments\t%d\n", issue.comments);
}

int
main(int argc, char **argv)
{
    if (strcmp(argv[1], "issue") == 0)
        issue();
    else
        return 1;
    return 0;
}
