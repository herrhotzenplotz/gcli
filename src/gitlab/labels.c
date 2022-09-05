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

#include <gcli/gitlab/config.h>
#include <gcli/gitlab/labels.h>
#include <gcli/json_util.h>

#include <pdjson/pdjson.h>

static uint32_t
gitlab_get_color(struct json_stream *input)
{
	char *color  = get_string(input);
	char *endptr = NULL;
	long  code   = 0;

	code = strtol(color + 1, &endptr, 16);
	if (endptr != (color + 1 + strlen(color + 1)))
		err(1, "error: color code is invalid");

	free(color);

	return ((uint32_t)(code) << 8);
}

static void
gitlab_parse_label(struct json_stream *input, gcli_label *it)
{
	if (json_next(input) != JSON_OBJECT)
		errx(1, "expected label object");

	while (json_next(input) == JSON_STRING) {
		size_t      len = 0;
		const char *key = json_get_string(input, &len);

		if (strncmp("name", key, len) == 0)
			it->name = get_string(input);
		else if (strncmp("description", key, len) == 0)
			it->description = get_string(input);
		else if (strncmp("color", key, len) == 0)
			it->color = gitlab_get_color(input);
		else if (strncmp("id", key, len) == 0)
			it->id = get_int(input);
		else
			SKIP_OBJECT_VALUE(input);
	}
}

size_t
gitlab_get_labels(
	const char   *owner,
	const char   *repo,
	int           max,
	gcli_label **out)
{
	size_t              out_size = 0;
	char               *url      = NULL;
	char               *next_url = NULL;
	gcli_fetch_buffer  buffer   = {0};
	struct json_stream  stream   = {0};

	*out = NULL;

	url = sn_asprintf("%s/projects/%s%%2F%s/labels",
			  gitlab_get_apibase(), owner, repo);

	do {
		enum json_type next_type;

		gcli_fetch(url, &next_url, &buffer);

		json_open_buffer(&stream, buffer.data, buffer.length);
		json_set_streaming(&stream, 1);

		next_type = json_next(&stream);
		if (next_type != JSON_ARRAY)
			errx(1, "error: expected array of labels");

		while (json_peek(&stream) != JSON_ARRAY_END) {
			gcli_label *it;

			*out = realloc(*out, sizeof(gcli_label) * (out_size + 1));
			it = &(*out)[out_size++];
			gitlab_parse_label(&stream, it);
		}

		free(buffer.data);
		free(url);
		json_close(&stream);
	} while ((url = next_url) && (max == -1 || (int)out_size < max));

	return out_size;
}

void
gitlab_create_label(const char *owner, const char *repo, gcli_label *label)
{
	char				*url			= NULL;
	char				*data			= NULL;
	char                *colour_string  = NULL;
	sn_sv				 lname_escaped	= SV_NULL;
	sn_sv				 ldesc_escaped	= SV_NULL;
	gcli_fetch_buffer	 buffer			= {0};
	struct json_stream	 stream			= {0};

	url = sn_asprintf("%s/projects/%s%%2F%s/labels",
			  gitlab_get_apibase(),
			  owner, repo);
	lname_escaped = gcli_json_escape(SV(label->name));
	ldesc_escaped = gcli_json_escape(SV(label->description));
	colour_string = sn_asprintf("%06X", (label->color>>8)&0xFFFFFF);
	data = sn_asprintf(
		"{\"name\": \""SV_FMT"\","
		"\"color\":\"#%s\","
		"\"description\":\""SV_FMT"\"}",
		SV_ARGS(lname_escaped),
		colour_string,
		SV_ARGS(ldesc_escaped));

	gcli_fetch_with_method("POST", url, data, NULL, &buffer);

	json_open_buffer(&stream, buffer.data, buffer.length);
	json_set_streaming(&stream, 1);

	gitlab_parse_label(&stream, label);

	json_close(&stream);
	free(lname_escaped.data);
	free(ldesc_escaped.data);
	free(colour_string);
	free(data);
	free(url);
	free(buffer.data);
}

void
gitlab_delete_label(
	const char *owner,
	const char *repo,
	const char *label)
{
	char               *url     = NULL;
	char               *e_label = NULL;
	gcli_fetch_buffer  buffer  = {0};

	e_label = gcli_urlencode(label);
	url     = sn_asprintf("%s/projects/%s%%2F%s/labels/%s",
				  gitlab_get_apibase(),
				  owner, repo, e_label);

	gcli_fetch_with_method("DELETE", url, NULL, NULL, &buffer);
	free(url);
	free(buffer.data);
	free(e_label);
}
