/*
 * Copyright Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/bugzilla/attachments.h>

#include <gcli/base64.h>
#include <gcli/curl.h>
#include <gcli/json_gen.h>

#include <gcli/port/string.h>

#include <templates/bugzilla/bugs.h>

int
bugzilla_attachment_get_content(struct gcli_ctx *ctx,
                                struct gcli_path const *path,
                                FILE *output)
{
	int rc = 0;
	char *url;
	struct gcli_fetch_buffer buffer = {0};
	struct json_stream stream = {0};
	struct gcli_attachment attachment = {0};

	if (path->kind != GCLI_PATH_ID) {
		return gcli_error(ctx,
		                  "unsupported path kind for attachment, "
		                  "requires ID");
	}

	url = gcli_asprintf("%s/rest/bug/attachment/%"PRIid,
	                    gcli_get_apibase(ctx), path->as_id);

	rc = gcli_fetch(ctx, url, NULL, &buffer);
	if (rc < 0)
		goto error_fetch;

	json_open_buffer(&stream, buffer.data, buffer.length);
	rc = parse_bugzilla_attachment_content(ctx, &stream, &attachment);
	if (rc < 0)
		goto error_parse;

	rc = gcli_base64_decode_print(ctx, output, attachment.data_base64);

	gcli_attachment_free(&attachment);

error_parse:
	json_close(&stream);
	gcli_fetch_buffer_free(&buffer);

error_fetch:
	gcli_clear_ptr(&url);

	return rc;
}

static int
check_required_params(struct gcli_ctx *ctx,
                      struct gcli_attachment_create_opts const *opts)
{
	if (!opts->data)
		return gcli_error(ctx, "missing data payload");

	if (!opts->file_name)
		return gcli_error(ctx, "missing file name");

	if (opts->summary == 0)
		return gcli_error(ctx, "missing summary");

	if (!opts->content_type && !opts->is_patch)
		return gcli_error(ctx, "missing content_type");

	return 0;
}

static int
opts_to_json(struct gcli_ctx *ctx,
             struct gcli_attachment_create_opts const *opts,
             char **out)
{
	struct gcli_jsongen gen = {0};
	char *datab64 = NULL;
	char const *content_type, *token;
	int rc = 0;

	/* we must have an api token! */
	token = gcli_get_token(ctx);
	if (!token)
		return gcli_error(ctx, "creating attachments on bugzilla requires a token");

	/* required params */
	rc = check_required_params(ctx, opts);
	if (rc < 0)
		return rc;

	/* Prepare payload */
	rc = gcli_encode_base64(ctx, opts->data, opts->data_size, &datab64);
	if (rc < 0)
		return gcli_error(ctx, "failed to base64-encode data");

	/* Determine content type */
	if (!opts->content_type) {
		if (opts->is_patch)
			content_type = "text/plain";
		else
			content_type = "application/octet-stream";
	} else {
		content_type = opts->content_type;
	}

 	gcli_jsongen_init(&gen);
	gcli_jsongen_begin_object(&gen);
	{
		gcli_jsongen_objmember(&gen, "is_patch");
		gcli_jsongen_bool(&gen, opts->is_patch);

		gcli_jsongen_objmember(&gen, "is_private");
		gcli_jsongen_bool(&gen, opts->is_private);

		if (opts->comment) {
			gcli_jsongen_objmember(&gen, "comment");
			gcli_jsongen_string(&gen, opts->comment);
		}

		gcli_jsongen_objmember(&gen, "summary");
		gcli_jsongen_string(&gen, opts->summary);

		gcli_jsongen_objmember(&gen, "content_type");
		gcli_jsongen_string(&gen, content_type);

		gcli_jsongen_objmember(&gen, "file_name");
		gcli_jsongen_string(&gen, opts->file_name);

		gcli_jsongen_objmember(&gen, "data");
		gcli_jsongen_string(&gen, datab64);

		gcli_jsongen_objmember(&gen, "api_key");
		gcli_jsongen_string(&gen, token);
	}
	gcli_jsongen_end_object(&gen);

	*out = gcli_jsongen_to_string(&gen);

	gcli_clear_ptr(&datab64);

	return 0;
}

int
bugzilla_attachment_create(struct gcli_ctx *ctx,
                           struct gcli_attachment_create_opts const *opts)
{
	char *url = NULL, *payload = NULL;
	int rc = 0;

	if (opts->bug_id == 0)
		return gcli_error(ctx, "missing bug id");

	url = gcli_asprintf("%s/rest/bug/%"PRIid"/attachment",
	                    gcli_get_apibase(ctx), opts->bug_id);

	rc = opts_to_json(ctx, opts, &payload);
	if (rc < 0)
		return rc;

	/* TODO: parse response and return attachment IDs */
	rc = gcli_fetch_with_method(ctx, "POST", url, payload, NULL, NULL);
	if (rc < 0)
		return rc;

	gcli_clear_ptr(&url);
	gcli_clear_ptr(&payload);

	return 0;
}
