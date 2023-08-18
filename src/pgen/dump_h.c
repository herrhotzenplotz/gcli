/*
 * Copyright 2022 Nico Sonack <nsonack@herrhotzenplotz.de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <gcli/pgen.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static int
should_replace(char c)
{
	return c == '_' || c == '/' || c == '.' || c == '-';
}

static char *
get_header_name(void)
{
	size_t len;
	char *result;

	len = strlen(outfilename);
	result = calloc(len + 1, 1);

	for (size_t i = 0; i < len; ++i) {
		if (should_replace(outfilename[i]))
			result[i] = '_';
		else
			result[i] = toupper(outfilename[i]);
	}

	return result;
}

void
header_dump_h(void)
{
	char *hname = get_header_name();

	fprintf(outfile, "#ifndef %s\n", hname);
	fprintf(outfile, "#define %s\n\n", hname);

	fprintf(outfile, "#include <pdjson/pdjson.h>\n");
	free(hname);
}

void
objparser_dump_h(struct objparser *p)
{
	fprintf(outfile, "void parse_%s(gcli_ctx *ctx, struct json_stream *, %s *);\n",
	        p->name, p->returntype);
}

void
include_dump_h(const char *file)
{
	fprintf(outfile, "#include <%s>\n", file);
}

void
footer_dump_h(void)
{
	char *hname = get_header_name();

	fprintf(outfile, "\n#endif /* %s */\n", hname);

	free(hname);
}

void
arrayparser_dump_h(struct arrayparser *p)
{
	fprintf(outfile, "void parse_%s(gcli_ctx *ctx, struct json_stream *, "
	        "%s **out, size_t *out_size);\n",
	        p->name, p->returntype);
}
