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

#include <gcli/github/labels.h>
#include <gcli/config.h>
#include <gcli/json_util.h>

#include <pdjson/pdjson.h>

#include <templates/github/labels.h>

int
github_get_labels(char const *owner,
                  char const *reponame,
                  int const max,
                  gcli_label_list *const out)
{
	char *url = NULL;
	char *next_url = NULL;
	gcli_fetch_buffer buffer = {0};

	*out = (gcli_label_list) {0};

	url = sn_asprintf(
		"%s/repos/%s/%s/labels",
		gcli_get_apibase(), owner, reponame);

	do {
		struct json_stream stream = {0};

		gcli_fetch(url, &next_url, &buffer);
		json_open_buffer(&stream, buffer.data, buffer.length);
		parse_github_labels(&stream, &out->labels, &out->labels_size);

		json_close(&stream);
		free(url);
		free(buffer.data);
	} while ((url = next_url) && ((int)(out->labels_size) < max || max < 0));

	return 0;
}

int
github_create_label(char const *owner,
                    char const *repo,
                    gcli_label *const label)
{
	char               *url          = NULL;
	char               *data         = NULL;
	char               *e_owner      = NULL;
	char               *e_repo       = NULL;
	char               *colour       = NULL;
	sn_sv               label_name   = SV_NULL;
	sn_sv               label_descr  = SV_NULL;
	sn_sv               label_colour = SV_NULL;
	gcli_fetch_buffer   buffer       = {0};
	struct json_stream  stream       = {0};
	int                 rc           = 0;

	e_owner = gcli_urlencode(owner);
	e_repo  = gcli_urlencode(repo);

	colour = sn_asprintf("%06X", label->colour >> 8);

	label_name   = gcli_json_escape(SV(label->name));
	label_descr  = gcli_json_escape(SV(label->description));
	label_colour = gcli_json_escape(SV(colour));

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
	                   SV_ARGS(label_colour));

	rc = gcli_fetch_with_method("POST", url, data, NULL, &buffer);

	if (rc == 0) {
		json_open_buffer(&stream, buffer.data, buffer.length);
		parse_github_label(&stream, label);
		json_close(&stream);
	}

	free(url);
	free(data);
	free(e_owner);
	free(e_repo);
	free(colour);
	free(label_name.data);
	free(label_descr.data);
	free(label_colour.data);
	free(buffer.data);

	return rc;
}

int
github_delete_label(char const *owner,
                    char const *repo,
                    char const *label)
{
	char *url     = NULL;
	char *e_label = NULL;
	int   rc      = 0;

	e_label = gcli_urlencode(label);

	/* DELETE /repos/{owner}/{repo}/labels/{name} */
	url = sn_asprintf("%s/repos/%s/%s/labels/%s",
	                  gcli_get_apibase(),
	                  owner, repo, e_label);

	rc = gcli_fetch_with_method("DELETE", url, NULL, NULL, NULL);

	free(url);
	free(e_label);

	return rc;
}
