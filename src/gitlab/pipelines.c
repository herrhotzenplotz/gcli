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

#include <ghcli/color.h>
#include <ghcli/gitlab/config.h>
#include <ghcli/gitlab/pipelines.h>
#include <ghcli/json_util.h>
#include <pdjson/pdjson.h>
#include <sn/sn.h>

#include <assert.h>

static void
gitlab_parse_pipeline(struct json_stream *stream, gitlab_pipeline *out)
{
	if (json_next(stream) != JSON_OBJECT)
		errx(1, "error: expected pipeline object");

	while (json_next(stream) == JSON_STRING) {
		size_t      len = 0;
		const char *key = json_get_string(stream, &len);

		if (strncmp("status", key, len) == 0)
			out->status = get_string(stream);
		else if (strncmp("created_at", key, len) == 0)
			out->created_at = get_string(stream);
		else if (strncmp("updated_at", key, len) == 0)
			out->updated_at = get_string(stream);
		else if (strncmp("ref", key, len) == 0)
			out->ref = get_string(stream);
		else if (strncmp("sha", key, len) == 0)
			out->sha = get_string(stream);
		else if (strncmp("source", key, len) == 0)
			out->source = get_string(stream);
		else if (strncmp("id", key, len) == 0)
			out->id = get_int(stream);
		else
			SKIP_OBJECT_VALUE(stream);
	}
}

int
gitlab_get_pipelines(
	const char		 *owner,
	const char		 *repo,
	int				  max,
	gitlab_pipeline **out)
{
	char				*url	  = NULL;
	char				*next_url = NULL;
	ghcli_fetch_buffer	 buffer	  = {0};
	struct json_stream   stream   = {0};
	enum   json_type     next     = JSON_NULL;
	int                  out_size = 0;

	assert(out);
	*out = NULL;

	url = sn_asprintf("%s/projects/%s%%2F%s/pipelines",
					  gitlab_get_apibase(), owner, repo);

	do {
		ghcli_fetch(url, &next_url, &buffer);

		json_open_buffer(&stream, buffer.data, buffer.length);
		json_set_streaming(&stream, 1);

		next = json_next(&stream);
		if (next != JSON_ARRAY)
			errx(1, "error: expected list of pipelines");

		while ((next = json_peek(&stream)) == JSON_OBJECT) {
			gitlab_pipeline *it = NULL;

			*out = realloc(*out, sizeof(gitlab_pipeline) * (out_size + 1));
			it = &(*out)[out_size++];
			gitlab_parse_pipeline(&stream, it);

			if (out_size == max) {
				free(next_url);
				break;
			}
		}

		json_close(&stream);
		free(buffer.data);
		free(url);
	} while ((url = next_url) && (max == -1 || out_size < max));

	return out_size;
}

void
gitlab_print_pipelines(
	gitlab_pipeline *pipelines,
	int              pipelines_size)
{
	printf("%10.10s  %10.10s  %16.16s  %16.16s %-s\n",
		   "ID", "STATUS", "CREATED", "UPDATED", "REF");
	for (int i = 0; i < pipelines_size; ++i) {
		printf("%10ld  %s%10.10s%s  %16.16s  %16.16s %-s\n",
			   pipelines[i].id,
			   ghcli_state_color_str(pipelines[i].status),
			   pipelines[i].status,
			   ghcli_resetcolor(),
			   pipelines[i].created_at,
			   pipelines[i].updated_at,
			   pipelines[i].ref);
	}
}

void
gitlab_free_pipelines(
	gitlab_pipeline *pipelines,
	int              pipelines_size)
{
	for (int i = 0; i < pipelines_size; ++i) {
		free(pipelines[i].status);
		free(pipelines[i].created_at);
		free(pipelines[i].updated_at);
		free(pipelines[i].ref);
		free(pipelines[i].sha);
		free(pipelines[i].source);
	}
	free(pipelines);
}

void
gitlab_pipelines(const char *owner, const char *repo, int count)
{
	gitlab_pipeline *pipelines = NULL;
	int pipelines_size = gitlab_get_pipelines(
		owner, repo, count, &pipelines);

	gitlab_print_pipelines(pipelines, pipelines_size);
	gitlab_free_pipelines(pipelines, pipelines_size);
}

void
gitlab_pipeline_jobs(const char *owner, const char *repo, long id, int count)
{
	gitlab_job *jobs = NULL;
	int jobs_size = gitlab_get_pipeline_jobs(owner, repo, id, count, &jobs);
	gitlab_print_jobs(jobs, jobs_size);
	gitlab_free_jobs(jobs, jobs_size);
}

static void
gitlab_parse_job_runner(struct json_stream *stream, gitlab_job *out)
{
	enum json_type next = json_next(stream);
	if (next == JSON_NULL)
		goto out;

	if (next != JSON_OBJECT)
		errx(1, "error: expected job runner object");

	while (json_next(stream) == JSON_STRING) {
		size_t      len = 0;
		const char *key = json_get_string(stream, &len);

		if (strncmp("name", key, len) == 0)
			out->runner_name = get_string(stream);
		else if (strncmp("description", key, len) == 0)
			out->runner_description = get_string(stream);
		else
			SKIP_OBJECT_VALUE(stream);
	}

out:
	/* Hack to prevent null pointers passed into printf */
	if (!out->runner_name)
		out->runner_name = strdup("<empty>");
	if (!out->runner_description)
		out->runner_description = strdup("<empty>");
}

static void
gitlab_parse_job(struct json_stream *stream, gitlab_job *out)
{
	if (json_next(stream) != JSON_OBJECT)
		errx(1, "error: expected job object");

	while (json_next(stream) == JSON_STRING) {
		size_t      len = 0;
		const char *key = json_get_string(stream, &len);

		if (strncmp("status", key, len) == 0)
			out->status = get_string(stream);
		else if (strncmp("stage", key, len) == 0)
			out->stage = get_string(stream);
		else if (strncmp("name", key, len) == 0)
			out->name = get_string(stream);
		else if (strncmp("ref", key, len) == 0)
			out->ref = get_string(stream);
		else if (strncmp("created_at", key, len) == 0)
			out->created_at = get_string(stream);
		else if (strncmp("started_at", key, len) == 0)
			out->started_at = get_string(stream);
		else if (strncmp("finished_at", key, len) == 0)
			out->finished_at = get_string(stream);
		else if (strncmp("runner", key, len) == 0)
			gitlab_parse_job_runner(stream, out);
		else if (strncmp("duration", key, len) == 0)
			out->duration = get_double(stream);
		else if (strncmp("id", key, len) == 0)
			out->id = get_int(stream);
		else
			SKIP_OBJECT_VALUE(stream);
	}
}

int
gitlab_get_pipeline_jobs(
	const char	 *owner,
	const char	 *repo,
	long		  pipeline,
	int			  max,
	gitlab_job	**out)
{
	char	*url	  = NULL;
	char	*next_url = NULL;
	int		 out_size = 0;

	url = sn_asprintf("%s/projects/%s%%2F%s/pipelines/%ld/jobs",
					  gitlab_get_apibase(), owner, repo, pipeline);

	do {
		ghcli_fetch_buffer	buffer = {0};
		struct json_stream	stream = {0};
		enum json_type		next   = JSON_NULL;

		ghcli_fetch(url, &next_url, &buffer);
		json_open_buffer(&stream, buffer.data, buffer.length);
		json_set_streaming(&stream, 1);

		next = json_next(&stream);
		if (next != JSON_ARRAY)
			errx(1, "error: expected array of jobs");

		while (json_peek(&stream) == JSON_OBJECT) {
			gitlab_job *it = NULL;
			*out = realloc(*out, sizeof(gitlab_job) * (out_size + 1));

			it = &(*out)[out_size++];
			memset(it, 0, sizeof(gitlab_job));
			gitlab_parse_job(&stream, it);
		}

	} while ((next_url = url) && (out_size < max || max == -1), false);

	return out_size;
}

void
gitlab_print_jobs(gitlab_job *jobs, int jobs_size)
{
	printf("%10.10s  %10.10s  %10.10s  %16.16s  %16.16s  %12.12s  %-s\n",
		   "ID", "NAME", "STATUS", "STARTED", "FINISHED", "RUNNERDESC", "REF");
	for (int i = 0; i < jobs_size; ++i) {
		printf("%10ld  %10.10s  %s%10.10s%s  %16.16s  %16.16s  %12.12s  %-s\n",
			   jobs[i].id,
			   jobs[i].name,
			   ghcli_state_color_str(jobs[i].status),
			   jobs[i].status,
			   ghcli_resetcolor(),
			   jobs[i].started_at,
			   jobs[i].finished_at,
			   jobs[i].runner_description,
			   jobs[i].ref);
	}
}

static void
gitlab_free_job_data(gitlab_job *job)
{
	free(job->status);
	free(job->stage);
	free(job->name);
	free(job->ref);
	free(job->created_at);
	free(job->started_at);
	free(job->finished_at);
	free(job->runner_name);
	free(job->runner_description);
}

void
gitlab_free_jobs(gitlab_job	*jobs, int jobs_size)
{
	for (int i = 0; i < jobs_size; ++i) {
		gitlab_free_job_data(&jobs[i]);
	}
	free(jobs);
}

void
gitlab_job_get_log(const char *owner, const char *repo, long job_id)
{
	ghcli_fetch_buffer	 buffer = {0};
	char				*url	= NULL;

	url = sn_asprintf("%s/projects/%s%%2F%s/jobs/%ld/trace",
					  gitlab_get_apibase(), owner, repo, job_id);

	ghcli_fetch(url, NULL, &buffer);

	fwrite(buffer.data, buffer.length, 1, stdout);

	free(buffer.data);
	free(url);
}

static void
gitlab_get_job(const char *owner, const char *repo, long jid, gitlab_job *out)
{
	ghcli_fetch_buffer	 buffer = {0};
	char				*url	= NULL;
	struct json_stream	 stream = {0};

	url = sn_asprintf("%s/projects/%s%%2F%s/jobs/%ld",
					  gitlab_get_apibase(), owner, repo, jid);
	ghcli_fetch(url, NULL, &buffer);

	json_open_buffer(&stream, buffer.data, buffer.length);
	json_set_streaming(&stream, 1);

	gitlab_parse_job(&stream, out);

	free(buffer.data);
	free(url);
	json_close(&stream);
}

static void
gitlab_print_job_status(gitlab_job *job)
{
	printf("          ID : %ld\n",     job->id);
	printf("      STATUS : %s%s%s\n",
		   ghcli_state_color_str(job->status),
		   job->status,
		   ghcli_resetcolor());
	printf("       STAGE : %s\n",      job->stage);
	printf("        NAME : %s\n",      job->name);
	printf("         REF : %s%s%s\n",
		   ghcli_setcolor(GHCLI_COLOR_YELLOW),
		   job->ref,
		   ghcli_resetcolor());
	printf("     CREATED : %s\n",      job->created_at);
	printf("     STARTED : %s\n",      job->started_at);
	printf("    FINISHED : %s\n",      job->finished_at);
	printf("    DURATION : %-.2lfs\n", job->duration);
	printf(" RUNNER NAME : %s\n",      job->runner_name);
	printf("RUNNER DESCR : %s\n",      job->runner_description);
}

void
gitlab_job_status(const char *owner, const char *repo, long jid)
{
	gitlab_job job = {0};

	gitlab_get_job(owner, repo, jid, &job);
	gitlab_print_job_status(&job);
	gitlab_free_job_data(&job);
}
