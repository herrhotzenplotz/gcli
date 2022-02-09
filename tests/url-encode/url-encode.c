#include <stdio.h>
#include <ghcli/curl.h>

int
main(void)
{
    char  text[]  = "some-random url// with %%%%%content"
        "Rindfleischettikettierungsüberwachungsaufgabenübertragungsgesetz";
    char *escaped = ghcli_url_encode(text);
    printf("%s\n", escaped);
    return 0;
}
