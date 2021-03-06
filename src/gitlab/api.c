/*
 * Copyright 2021 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <gcli/gitlab/api.h>
#include <gcli/json_util.h>

#include <pdjson/pdjson.h>

const char *
gitlab_api_error_string(gcli_fetch_buffer *it)
{
	struct json_stream stream = {0};
	enum json_type     next   = JSON_NULL;

	if (!it->length)
		return NULL;

	json_open_buffer(&stream, it->data, it->length);
	json_set_streaming(&stream, true);

	while ((next = json_next(&stream)) != JSON_OBJECT_END) {
		char *key = get_string(&stream);
		if (strcmp(key, "message") == 0) {
			if ((next = json_peek(&stream)) == JSON_STRING)
				return get_string(&stream);
			else if ((next = json_next(&stream)) == JSON_ARRAY)
				return get_string(&stream);
			else if (next == JSON_ARRAY_END)
				;
			else
				errx(1, "error retrieving error message from GitLab API response");
		}
		free(key);
	}

	return "<No message key in error response object>";
}
