#include <stdio.h>
#include <stdlib.h>

#include <gcli/curl.h>

int
torture(void)
{
	char  text[]  = "some-random url// with %%%%%content"
		"Rindfleischettikettierungsüberwachungsaufgabenübertragungsgesetz";
	char *escaped = gcli_urlencode(text);

	return strcmp(
		escaped,
		"some-random%20url%2F%2F%20with%20%25%25%25%25%25contentRindfleisch"
		"ettikettierungs%C3%BCberwachungsaufgaben%C3%BCbertragungsgesetz");
}

int
main(void)
{
	if (strcmp(gcli_urlencode("%"), "%25"))
		return 1;

	if (strcmp(gcli_urlencode(" "), "%20"))
		return 1;

	if (strcmp(gcli_urlencode("ä"), "%C3%A4"))
		return 1;

	if (strcmp(gcli_urlencode("ẞ"), "%E1%BA%9E"))
		return 1;

	if (strcmp(gcli_urlencode("-"), "-"))
		return 1;

	if (strcmp(gcli_urlencode("_"), "_"))
		return 1;

	if (torture())
		return 1;

	return 0;
}
