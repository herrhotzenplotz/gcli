/*
 * Copyright 2025 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/bugzilla/comment.h>

#include <gcli/curl.h>
#include <gcli/json_gen.h>

#include <gcli/port/string.h>

int
bugzilla_submit_comment(struct gcli_ctx *ctx,
                        struct gcli_submit_comment_opts const *const opts)
{
	char *url = NULL, *payload = NULL, *token = NULL;
	int rc = 0;
	struct gcli_jsongen gen = {0};
	struct gcli_path const *const tgt = &opts->target;

	if (tgt->kind != GCLI_PATH_ID) {
		return gcli_error(
			ctx,
			"bad path kind for submitting Bugzilla comment"
		);
	}

	/* grab a valid API token */
	token = gcli_get_token(ctx);
	if (!token)
		return gcli_error(ctx, "creating comments on bugzilla requires a token");

	/* construct URL */
	url = gcli_asprintf("%s/rest/bug/%"PRIid"/comment",
	                    gcli_get_apibase(ctx), tgt->as_id);

	/* construct payload */
	gcli_jsongen_init(&gen);
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, "comment");
		gcli_jsongen_string(&gen, opts->message);

		gcli_jsongen_objmember(&gen, "is_private");
		gcli_jsongen_bool(&gen, false);

		gcli_jsongen_objmember(&gen, "api_key");
		gcli_jsongen_string(&gen, token);
	}
	gcli_jsongen_end_object(&gen);

	payload = gcli_jsongen_to_string(&gen);
	gcli_jsongen_free(&gen);

	/* perform request */
	rc = gcli_fetch_with_method(ctx, "POST", url, payload, NULL, NULL);

	/* cleanup */
	gcli_clear_ptr(&url);
	gcli_clear_ptr(&payload);
	gcli_clear_ptr(&token);

	return rc;
}
