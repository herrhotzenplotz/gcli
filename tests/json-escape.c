#include <stdio.h>
#include <stdlib.h>

#include <gcli/json_util.h>

int
main(void)
{
    if (!sn_sv_eq_to(
			gcli_json_escape(SV("\n\r\n\n\n\t{}")),
			"\\n\\r\\n\\n\\n\\t{}"))
        return 1;

    if (!sn_sv_eq_to(
			gcli_json_escape(SV("\\")),
			"\\\\"))
        return 1;

    return 0;
}
