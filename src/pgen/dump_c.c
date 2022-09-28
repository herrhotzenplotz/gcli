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

void
objparser_dump_c(struct objparser *p)
{
    fprintf(outfile,
            "void parse_%s(struct json_stream *stream, struct %s *out)\n",
            p->name, p->returntype);
    fprintf(outfile, "{\n");
    fprintf(outfile, "\tenum json_type key_type;\n");
    fprintf(outfile, "\tconst char *key;\n\n");
    fprintf(outfile, "\tjson_next(stream);\n\n");
    fprintf(outfile, "\twhile ((key_type = json_next(stream)) == JSON_STRING) {\n");
    fprintf(outfile, "\t\tsize_t len;\n");
    fprintf(outfile, "\t\tkey = json_get_string(stream, &len);\n");

    for (struct objentry *it = p->entries; it; it = it->next)
    {
        fprintf(outfile, "\t\tif (strncmp(\"%s\", key, len) == 0)\n", it->jsonname);

        if (it->kind == OBJENTRY_SIMPLE) {

            if (it->parser)
                fprintf(outfile, "\t\t\t%s(stream, &out->%s);\n", it->parser, it->name);
            else
                fprintf(outfile, "\t\t\tout->%s = get_%s(stream);\n", it->name, it->type);

        } else if (it->kind = OBJENTRY_ARRAY) {
            fprintf(outfile, "\t\t\tparse_%s_%s_array(stream, out);\n",
                    p->name, it->name);
            /* TODO: generate these functions */
        }
        fprintf(outfile, "\t\telse ");
    }

    fprintf(outfile, "\n\t\t\tSKIP_OBJECT_VALUE(stream);\n");

    fprintf(outfile, "\t}\n");
    fprintf(outfile, "\tif (key_type != JSON_OBJECT_END)\n");
    fprintf(outfile, "\t\terrx(1, \"unexpected object key type\");\n");
    fprintf(outfile, "}\n\n");
}
