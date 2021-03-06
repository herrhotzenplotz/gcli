/*
 * Copyright 2022 Nico Sonack <nsonack@outlook.com>
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

#include <gcli/github/labels.h>
#include <gcli/config.h>
#include <gcli/json_util.h>

#include <pdjson/pdjson.h>

static void
github_parse_label(struct json_stream *input, gcli_label *out)
{
	if (json_next(input) != JSON_OBJECT)
		errx(1, "Expected Issue Object");

	while (json_next(input) == JSON_STRING) {
		size_t      len = 0;
		const char *key = json_get_string(input, &len);

		if (strncmp("id", key, len) == 0)
			out->id = get_int(input);
		else if (strncmp("name", key, len) == 0)
			out->name = get_string(input);
		else if (strncmp("description", key, len) == 0)
			out->description = get_string(input);
		else if (strncmp("color", key, len) == 0)
			out->color = get_github_style_color(input);
		else
			SKIP_OBJECT_VALUE(input);
	}
}

size_t
github_get_labels(
	const char   *owner,
	const char   *reponame,
	int           max,
	gcli_label **out)
{
	size_t              out_size = 0;
	char               *url      = NULL;
	char               *next_url = NULL;
	gcli_fetch_buffer  buffer   = {0};

	*out = NULL;

	url = sn_asprintf(
		"%s/repos/%s/%s/labels",
		gcli_get_apibase(), owner, reponame);

	do {
		struct json_stream stream = {0};
		enum   json_type   next   = JSON_NULL;

		gcli_fetch(url, &next_url, &buffer);
		json_open_buffer(&stream, buffer.data, buffer.length);
		json_set_streaming(&stream, 1);

		next = json_next(&stream);
		if (next != JSON_ARRAY)
			errx(1, "error: expected array of labels");

		while (json_peek(&stream) != JSON_ARRAY_END) {
			gcli_label *it = NULL;

			*out = realloc(*out, sizeof(gcli_label) * (out_size + 1));
			it = &(*out)[out_size++];

			memset(it, 0, sizeof(*it));
			github_parse_label(&stream, it);
		}

		json_close(&stream);
		free(url);
		free(buffer.data);
	} while ((url = next_url) && ((int)(out_size) < max || max < 0));

	return out_size;
}

void
github_create_label(
	const char  *owner,
	const char  *repo,
	gcli_label *label)
{
	char               *url         = NULL;
	char               *data        = NULL;
	char               *e_owner     = NULL;
	char               *e_repo      = NULL;
	char               *color       = NULL;
	sn_sv               label_name  = SV_NULL;
	sn_sv               label_descr = SV_NULL;
	sn_sv               label_color = SV_NULL;
	gcli_fetch_buffer  buffer      = {0};
	struct json_stream  stream      = {0};

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	color = sn_asprintf("%06X", label->color >> 8);

	label_name  = gcli_json_escape(SV(label->name));
	label_descr = gcli_json_escape(SV(label->description));
	label_color = gcli_json_escape(SV(color));

	/* /repos/{owner}/{repo}/labels */
	url = sn_asprintf("%s/repos/%s/%s/labels",
			  gcli_get_apibase(), e_owner, e_repo);


	data = sn_asprintf("{ "
			   "  \"name\": \""SV_FMT"\", "
			   "  \"description\": \""SV_FMT"\", "
			   "  \"color\": \""SV_FMT"\""
			   "}",
			   SV_ARGS(label_name),
			   SV_ARGS(label_descr),
			   SV_ARGS(label_color));

	gcli_fetch_with_method("POST", url, data, NULL, &buffer);
	json_open_buffer(&stream, buffer.data, buffer.length);
	github_parse_label(&stream, label);

	json_close(&stream);
	free(url);
	free(data);
	free(e_owner);
	free(e_repo);
	free(color);
	free(label_name.data);
	free(label_descr.data);
	free(label_color.data);
	free(buffer.data);
}

void
github_delete_label(
	const char *owner,
	const char *repo,
	const char *label)
{
	char               *url     = NULL;
	char               *e_label = NULL;
	gcli_fetch_buffer  buffer  = {0};

	e_label = gcli_urlencode(label);

	/* DELETE /repos/{owner}/{repo}/labels/{name} */
	url = sn_asprintf("%s/repos/%s/%s/labels/%s",
			  gcli_get_apibase(),
			  owner, repo, e_label);

	gcli_fetch_with_method("DELETE", url, NULL, NULL, &buffer);

	free(url);
	free(e_label);
	free(buffer.data);
}
