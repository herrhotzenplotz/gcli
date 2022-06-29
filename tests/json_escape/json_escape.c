#include <stdio.h>
#include <gcli/json_util.h>

int
main(void)
{
	char  text[]  = "\n\r\n\n\n\t{}";
	sn_sv escaped = gcli_json_escape(SV(text));
	printf(SV_FMT"\n", SV_ARGS(escaped));
	return 0;
}
