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
#include <gcli/json_util.h>

#include <assert.h>

int
bugzilla_get_bugs(gcli_ctx *ctx, char const *product, char const *component,
                  gcli_issue_fetch_details const *details, int const max,
                  gcli_issue_list *out)
{
	char *url, *e_product = NULL, *e_component = NULL, *e_author = NULL;
	gcli_fetch_buffer buffer = {0};
	int rc = 0;

	if (product) {
		char *tmp = gcli_urlencode(product);
		e_product = sn_asprintf("&product=%s", tmp);
		free(tmp);
	}

	if (component) {
		char *tmp = gcli_urlencode(component);
		e_component = sn_asprintf("&component=%s", tmp);
		free(tmp);
	}

	if (details->author) {
		char *tmp = gcli_urlencode(details->author);
		e_author = sn_asprintf("&creator=%s", tmp);
		free(tmp);
	}

	/* TODO: handle the max = -1 case */
	/* Note(Nico): Most of the options here are not very well
	 * documented. Specifically the order= parameter I have figured out by
	 * reading the code and trying things until it worked. */
	url = sn_asprintf("%s/rest/bug?order=bug_id%%20DESC%%2C&limit=%d%s%s%s%s",
	                  gcli_get_apibase(ctx), max,
	                  details->all ? "&status=All" : "&status=Open&status=New",
	                  e_product ? e_product : "",
	                  e_component ? e_component : "",
	                  e_author ? e_author : "");

	free(e_product);
	free(e_component);
	free(e_author);

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

int
parse_bugzilla_comments_array_skip_first(gcli_ctx *ctx,
                                         struct json_stream *stream,
                                         gcli_comment_list *out)
{
	int rc = 0;

	if (json_next(stream) != JSON_ARRAY)
		return gcli_error(ctx, "expected array for comments array");

	SKIP_OBJECT_VALUE(stream);

	while (json_peek(stream) != JSON_ARRAY_END) {
		out->comments = realloc(out->comments, sizeof(*out->comments) * (out->comments_size + 1));
		memset(&out->comments[out->comments_size], 0, sizeof(out->comments[out->comments_size]));
		rc = parse_bugzilla_comment(ctx, stream, &out->comments[out->comments_size++]);
		if (rc < 0)
			return rc;
	}

	if (json_next(stream) != JSON_ARRAY_END)
		return gcli_error(ctx, "unexpected element in array while parsing");

	return 0;
}

int
parse_bugzilla_comments_array_only_first(gcli_ctx *ctx,
                                         struct json_stream *stream,
                                         char **out)
{
	int rc = 0;

	if (json_next(stream) != JSON_ARRAY)
		return gcli_error(ctx, "expected array for comments array");

	rc = parse_bugzilla_comment_text(ctx, stream, out);
	if (rc < 0)
		return rc;

	while (json_peek(stream) != JSON_ARRAY_END) {
		SKIP_OBJECT_VALUE(stream);
	}

	if (json_next(stream) != JSON_ARRAY_END)
		return gcli_error(ctx, "unexpected element in array while parsing");

	return 0;
}

int
parse_bugzilla_bug_comments_dictionary_skip_first(gcli_ctx *const ctx,
                                                  json_stream *stream,
                                                  gcli_comment_list *out)
{
	enum json_type next = JSON_NULL;
	int rc = 0;

	if ((next = json_next(stream)) != JSON_OBJECT)
		return gcli_error(ctx, "expected bugzilla comments dictionary");

	while ((next = json_next(stream)) == JSON_STRING) {
		rc = parse_bugzilla_comments_internal_skip_first(ctx, stream, out);
		if (rc < 0)
			return rc;
	}

	if (next != JSON_OBJECT_END)
		return gcli_error(ctx, "unclosed bugzilla comments dictionary");

	return rc;
}

int
parse_bugzilla_bug_comments_dictionary_only_first(gcli_ctx *const ctx,
                                                  json_stream *stream,
                                                  char **out)
{
	enum json_type next = JSON_NULL;
	int rc = 0;

	if ((next = json_next(stream)) != JSON_OBJECT)
		return gcli_error(ctx, "expected bugzilla comments dictionary");

	while ((next = json_next(stream)) == JSON_STRING) {
		rc = parse_bugzilla_comments_internal_only_first(ctx, stream, out);
		if (rc < 0)
			return rc;
	}

	if (next != JSON_OBJECT_END)
		return gcli_error(ctx, "unclosed bugzilla comments dictionary");

	return rc;
}

int
bugzilla_bug_get_comments(gcli_ctx *const ctx, char const *const product,
                          char const *const component, gcli_id const bug_id,
                          gcli_comment_list *out)
{
	int rc = 0;
	gcli_fetch_buffer buffer = {0};
	json_stream stream = {0};
	char *url = NULL;

	(void) product;
	(void) component;

	url = sn_asprintf("%s/rest/bug/%"PRIid"/comment?include_fields=_all",
	                  gcli_get_apibase(ctx), bug_id);

	rc = gcli_fetch(ctx, url, NULL, &buffer);
	if (rc < 0)
		goto error_fetch;

	json_open_buffer(&stream, buffer.data, buffer.length);
	rc = parse_bugzilla_comments(ctx, &stream, out);
	json_close(&stream);

	free(buffer.data);

error_fetch:
	free(url);

	return rc;
}

static int
bugzilla_bug_get_op(gcli_ctx *ctx, gcli_id const bug_id, char **out)
{
	int rc = 0;
	gcli_fetch_buffer buffer = {0};
	json_stream stream = {0};
	char *url = NULL;

	url = sn_asprintf("%s/rest/bug/%"PRIid"/comment?include_fields=_all",
	                  gcli_get_apibase(ctx), bug_id);

	rc = gcli_fetch(ctx, url, NULL, &buffer);
	if (rc < 0)
		goto error_fetch;

	json_open_buffer(&stream, buffer.data, buffer.length);
	rc = parse_bugzilla_bug_op(ctx, &stream, out);
	json_close(&stream);

	free(buffer.data);

error_fetch:
	free(url);

	return rc;
}

int
bugzilla_get_bug(gcli_ctx *ctx, char const *product, char const *component,
                 gcli_id bug_id, gcli_issue *out)
{
	int rc = 0;
	char *url;
	gcli_fetch_buffer buffer = {0};
	gcli_issue_list list = {0};
	json_stream stream = {0};

	/* XXX should we warn if product or component is set? */
	(void) product;
	(void) component;

	url = sn_asprintf("%s/rest/bug?limit=1&id=%"PRIid, gcli_get_apibase(ctx), bug_id);
	rc = gcli_fetch(ctx, url, NULL, &buffer);

	if (rc < 0)
		goto error_fetch;

	json_open_buffer(&stream, buffer.data, buffer.length);
	rc = parse_bugzilla_bugs(ctx, &stream, &list);

	if (rc < 0)
		goto error_parse;

	if (list.issues_size == 0) {
		rc = gcli_error(ctx, "no bug with id %"PRIid, bug_id);
		goto error_no_such_bug;
	}

	if (list.issues_size > 0) {
		assert(list.issues_size == 1);
		memcpy(out, &list.issues[0], sizeof(*out));
	}

	/* don't use gcli_issues_free because it frees data behind pointers we
	 * just copied */
	free(list.issues);

	/* The OP is in the comments. Fetch it separately. */
	rc = bugzilla_bug_get_op(ctx, bug_id, &out->body);

error_no_such_bug:
error_parse:
	json_close(&stream);
	free(buffer.data);

error_fetch:
	free(url);

	return rc;
}

int
parse_bugzilla_assignee(gcli_ctx *ctx, struct json_stream *stream,
                        gcli_issue *out)
{
	out->assignees = calloc(1, sizeof (*out->assignees));
	out->assignees_size = 1;

	return get_string(ctx, stream, out->assignees);
}

int
bugzilla_bug_get_attachments(gcli_ctx *ctx, char const *const product,
                             char const *const component, gcli_id const bug_id,
                             gcli_attachment_list *const out)
{
	int rc = 0;
	char *url = NULL;
	gcli_fetch_buffer buffer = {0};
	json_stream stream = {0};

	(void) product;
	(void) component;

	url = sn_asprintf("%s/rest/bug/%"PRIid"/attachment",
	                  gcli_get_apibase(ctx), bug_id);

	rc = gcli_fetch(ctx, url, NULL, &buffer);
	if (rc < 0)
		goto error_fetch;

	json_open_buffer(&stream, buffer.data, buffer.length);
	rc = parse_bugzilla_bug_attachments(ctx, &stream, out);
	json_close(&stream);

	free(buffer.data);

error_fetch:
	free(url);

	return rc;
}

int
parse_bugzilla_bug_attachments_dict(gcli_ctx *ctx, json_stream *stream,
                                    gcli_attachment_list *out)
{
	enum json_type next = JSON_NULL;
	int rc = 0;

	if ((next = json_next(stream)) != JSON_OBJECT)
		return gcli_error(ctx, "expected bugzilla attachments dictionary");

	while ((next = json_next(stream)) == JSON_STRING) {
		rc = parse_bugzilla_bug_attachments_internal(ctx, stream,
		                                             &out->attachments,
		                                             &out->attachments_size);
		if (rc < 0)
			return rc;
	}

	if (next != JSON_OBJECT_END)
		return gcli_error(ctx, "unclosed bugzilla attachments dictionary");

	return rc;
}
