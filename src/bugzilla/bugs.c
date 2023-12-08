/*
 * Copyright 2023 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <config.h>

#include <gcli/bugzilla/bugs.h>

#include <templates/bugzilla/bugs.h>

#include <gcli/curl.h>

int
bugzilla_get_bugs(gcli_ctx *ctx, char const *product, char const *component,
                  gcli_issue_fetch_details const *details, int const max,
                  gcli_issue_list *out)
{
	char *url;
	gcli_fetch_buffer buffer = {0};
	int rc = 0;

	(void) product;
	(void) component;
	(void) details;

	/* TODO: handle the max = -1 case */
	url = sn_asprintf("%s/rest/bug?limit=%d&%s&order=id%%20DESC",
	                  gcli_get_apibase(ctx), max,
	                  details->all ? "status=All" : "status=Open");
	rc = gcli_fetch(ctx, url, NULL, &buffer);
	if (rc == 0) {
		json_stream stream = {0};

		json_open_buffer(&stream, buffer.data, buffer.length);
		rc = parse_bugzilla_bugs(ctx, &stream, out);

		json_close(&stream);
	}

	free(buffer.data);
	free(url);

	return rc;
}
