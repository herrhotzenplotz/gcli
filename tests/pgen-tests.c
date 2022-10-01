#include <templates/github/issues.h>

#include <stdio.h>
#include <stdlib.h>

int
main(void)
{
    struct json_stream str   = {0};
    gcli_issue         issue = {0};

    json_open_stream(&str, stdin);
    parse_github_issue(&str, &issue);

    printf("title\t"SV_FMT"\n", SV_ARGS(issue.title));
    printf("number\t%d\n", issue.number);
    printf("author\t"SV_FMT"\n", SV_ARGS(issue.author));
    printf("locked\t%d\n", issue.locked);
    printf("state\t"SV_FMT"\n", SV_ARGS(issue.state));
    printf("labels_size\t%zu\n", issue.labels_size);
    printf("comments\t%d\n", issue.comments);

    return 0;
}
