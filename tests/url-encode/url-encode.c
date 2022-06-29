#include <stdio.h>
#include <gcli/curl.h>

int
main(void)
{
	char  text[]  = "some-random url// with %%%%%content"
		"Rindfleischettikettierungsüberwachungsaufgabenübertragungsgesetz";
	char *escaped = gcli_urlencode(text);
	printf("%s\n", escaped);
	return 0;
}
