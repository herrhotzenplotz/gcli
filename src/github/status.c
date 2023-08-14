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

#include <gcli/curl.h>
#include <gcli/github/status.h>
#include <gcli/config.h>
#include <gcli/json_util.h>

#include <sn/sn.h>
#include <pdjson/pdjson.h>

#include <templates/github/status.h>

int
github_get_notifications(gcli_ctx *ctx,
                         gcli_notification **const notifications,
                         int const count)
{
	char *url = NULL;
	char *next_url = NULL;
	gcli_fetch_buffer buffer = {0};
	struct json_stream stream = {0};
	size_t notifications_size = 0;
	int rc = 0;

	url = sn_asprintf("%s/notifications", gcli_get_apibase(ctx));

	do {
		rc = gcli_fetch(ctx, url, &next_url, &buffer);

		if (rc == 0) {
			json_open_buffer(&stream, buffer.data, buffer.length);
			parse_github_notifications(ctx, &stream, notifications,
			                           &notifications_size);
			json_close(&stream);
		}

		free(url);
		free(buffer.data);

		if (rc < 0)
			break;
	} while ((url = next_url) && (count < 0 || ((int)notifications_size < count)));

	/* TODO: don't leak the list on error */
	if (rc < 0)
		return rc;

	return (int)(notifications_size);
}

int
github_notification_mark_as_read(gcli_ctx *ctx, char const *id)
{
	char *url = NULL;
	int rc = 0;

	url = sn_asprintf(
		"%s/notifications/threads/%s",
		gcli_get_apibase(ctx),
		id);
	rc = gcli_fetch_with_method(ctx, "PATCH", url, NULL, NULL, NULL);

	free(url);

	return rc;
}
