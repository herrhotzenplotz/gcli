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

#include <gcli/color.h>
#include <gcli/gitlab/config.h>
#include <gcli/gitlab/pipelines.h>
#include <gcli/json_util.h>
#include <gcli/table.h>
#include <pdjson/pdjson.h>
#include <sn/sn.h>

#include <assert.h>

#include <templates/gitlab/pipelines.h>

static int
fetch_pipelines(char *url, int const max, gitlab_pipeline **const out)
{
	char               *next_url = NULL;
	gcli_fetch_buffer   buffer   = {0};
	struct json_stream  stream   = {0};
	size_t              out_size = 0;

	*out = NULL;

	do {
		gcli_fetch(url, &next_url, &buffer);

		json_open_buffer(&stream, buffer.data, buffer.length);

		parse_gitlab_pipelines(&stream, out, &out_size);

		json_close(&stream);
		free(buffer.data);
		free(url);
	} while ((url = next_url) && (max == -1 || (int)out_size < max));

	return (int)out_size;
}

int
gitlab_get_pipelines(char const *owner,
                     char const *repo,
                     int const max,
                     gitlab_pipeline **const out)
{
	char *url = NULL;

	url = sn_asprintf("%s/projects/%s%%2F%s/pipelines",
	                  gitlab_get_apibase(), owner, repo);

	return fetch_pipelines(url, max, out);
}

static int
gitlab_get_mr_pipelines(char const *owner, char const *repo, int const mr_id,
                        gitlab_pipeline **const out)
{
	char *url = NULL;

	url = sn_asprintf("%s/projects/%s%%2F%s/merge_requests/%d/pipelines",
	                  gitlab_get_apibase(), owner, repo, mr_id);

	/* fetch everything */
	return fetch_pipelines(url, -1, out);
}

void
gitlab_print_pipelines(gitlab_pipeline const *const pipelines,
                       int const pipelines_size)
{
	gcli_tbl table;
	gcli_tblcoldef cols[] = {
		{ .name = "ID",      .type = GCLI_TBLCOLTYPE_INT,    .flags = GCLI_TBLCOL_JUSTIFYR },
		{ .name = "STATUS",  .type = GCLI_TBLCOLTYPE_STRING, .flags = GCLI_TBLCOL_STATECOLOURED },
		{ .name = "CREATED", .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "UPDATED", .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "REF",     .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
	};

	if (!pipelines_size) {
		printf("No pipelines\n");
		return;
	}

	table = gcli_tbl_begin(cols, ARRAY_SIZE(cols));
	if (!table)
		errx(1, "error: could not init table");

	for (int i = 0; i < pipelines_size; ++i) {
		gcli_tbl_add_row(table,
		                 (int)(pipelines[i].id),
		                 pipelines[i].status,
		                 pipelines[i].created_at,
		                 pipelines[i].updated_at,
		                 pipelines[i].ref);
	}

	gcli_tbl_end(table);
}

void
gitlab_free_pipelines(gitlab_pipeline *pipelines,
                      int const pipelines_size)
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
gitlab_pipelines(char const *owner, char const *repo, int const count)
{
	gitlab_pipeline *pipelines = NULL;
	int const pipelines_size = gitlab_get_pipelines(
		owner, repo, count, &pipelines);

	gitlab_print_pipelines(pipelines, pipelines_size);
	gitlab_free_pipelines(pipelines, pipelines_size);
}

void
gitlab_pipeline_jobs(char const *owner,
                     char const *repo,
                     long const id,
                     int const count)
{
	gitlab_job *jobs = NULL;
	int const jobs_size = gitlab_get_pipeline_jobs(owner, repo, id, count, &jobs);
	gitlab_print_jobs(jobs, jobs_size);
	gitlab_free_jobs(jobs, jobs_size);
}

int
gitlab_get_pipeline_jobs(char const *owner,
                         char const *repo,
                         long const pipeline,
                         int const max,
                         gitlab_job **const out)
{
	char   *url      = NULL;
	char   *next_url = NULL;
	size_t  out_size = 0;

	url = sn_asprintf("%s/projects/%s%%2F%s/pipelines/%ld/jobs",
	                  gitlab_get_apibase(), owner, repo, pipeline);

	do {
		gcli_fetch_buffer  buffer = {0};
		struct json_stream stream = {0};

		gcli_fetch(url, &next_url, &buffer);
		json_open_buffer(&stream, buffer.data, buffer.length);

		parse_gitlab_jobs(&stream, out, &out_size);

		free(url);
	} while ((url = next_url) && (((int)out_size < max) || (max == -1)));

	return (int)out_size;
}

void
gitlab_print_jobs(gitlab_job const *const jobs, int const jobs_size)
{
	gcli_tbl table;
	gcli_tblcoldef cols[] = {
		{ .name = "ID",         .type = GCLI_TBLCOLTYPE_LONG,   .flags = GCLI_TBLCOL_JUSTIFYR },
		{ .name = "NAME",       .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "STATUS",     .type = GCLI_TBLCOLTYPE_STRING, .flags = GCLI_TBLCOL_STATECOLOURED },
		{ .name = "STARTED",    .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "FINISHED",   .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "RUNNERDESC", .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "REF",        .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
	};

	if (!jobs_size) {
		printf("No jobs\n");
		return;
	}

	table = gcli_tbl_begin(cols, ARRAY_SIZE(cols));
	if (!table)
		errx(1, "error: could not initialize table");

	for (int i = 0; i < jobs_size; ++i) {
		gcli_tbl_add_row(table,
		                 jobs[i].id,
		                 jobs[i].name,
		                 jobs[i].status,
		                 jobs[i].started_at,
		                 jobs[i].finished_at,
		                 jobs[i].runner_description,
		                 jobs[i].ref);
	}

	gcli_tbl_end(table);
}

static void
gitlab_free_job_data(gitlab_job *const job)
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
gitlab_free_jobs(gitlab_job *jobs, int const jobs_size)
{
	for (int i = 0; i < jobs_size; ++i) {
		gitlab_free_job_data(&jobs[i]);
	}
	free(jobs);
}

void
gitlab_job_get_log(char const *owner, char const *repo, long const job_id)
{
	gcli_fetch_buffer  buffer = {0};
	char              *url    = NULL;

	url = sn_asprintf("%s/projects/%s%%2F%s/jobs/%ld/trace",
	                  gitlab_get_apibase(), owner, repo, job_id);

	gcli_fetch(url, NULL, &buffer);

	fwrite(buffer.data, buffer.length, 1, stdout);

	free(buffer.data);
	free(url);
}

static void
gitlab_get_job(char const *owner,
               char const *repo,
               long const jid,
               gitlab_job *const out)
{
	gcli_fetch_buffer   buffer = {0};
	char               *url    = NULL;
	struct json_stream  stream = {0};

	url = sn_asprintf("%s/projects/%s%%2F%s/jobs/%ld",
	                  gitlab_get_apibase(), owner, repo, jid);
	gcli_fetch(url, NULL, &buffer);

	json_open_buffer(&stream, buffer.data, buffer.length);
	json_set_streaming(&stream, 1);

	parse_gitlab_job(&stream, out);

	free(buffer.data);
	free(url);
	json_close(&stream);
}

static void
gitlab_print_job_status(gitlab_job const *const job)
{
	printf("          ID : %ld\n",     job->id);
	printf("      STATUS : %s%s%s\n",
	       gcli_state_color_str(job->status),
	       job->status,
	       gcli_resetcolor());
	printf("       STAGE : %s\n",      job->stage);
	printf("        NAME : %s\n",      job->name);
	printf("         REF : %s%s%s\n",
	       gcli_setcolor(GCLI_COLOR_YELLOW),
	       job->ref,
	       gcli_resetcolor());
	printf("     CREATED : %s\n",      job->created_at);
	printf("     STARTED : %s\n",      job->started_at);
	printf("    FINISHED : %s\n",      job->finished_at);
	printf("    DURATION : %-.2lfs\n", job->duration);
	printf(" RUNNER NAME : %s\n",      job->runner_name);
	printf("RUNNER DESCR : %s\n",      job->runner_description);
}

void
gitlab_job_status(char const *owner, char const *repo, long const jid)
{
	gitlab_job job = {0};

	gitlab_get_job(owner, repo, jid, &job);
	gitlab_print_job_status(&job);
	gitlab_free_job_data(&job);
}

/* TODO: Maybe devise a macro for these things ? */
void
gitlab_job_cancel(char const *owner, char const *repo, long const jid)
{
	gcli_fetch_buffer  buffer = {0};
	char              *url    = NULL;

	url = sn_asprintf("%s/projects/%s%%2F%s/jobs/%ld/cancel",
	                  gitlab_get_apibase(), owner, repo, jid);
	gcli_fetch_with_method("POST", url, NULL, NULL, &buffer);

	free(url);
	free(buffer.data);
}

void
gitlab_job_retry(char const *owner, char const *repo, long const jid)
{
	gcli_fetch_buffer  buffer = {0};
	char              *url    = NULL;

	url = sn_asprintf("%s/projects/%s%%2F%s/jobs/%ld/retry",
	                  gitlab_get_apibase(), owner, repo, jid);
	gcli_fetch_with_method("POST", url, NULL, NULL, &buffer);

	free(url);
	free(buffer.data);
}

void
gitlab_mr_pipelines(char const *owner, char const *repo, int const mr_id)
{
	gitlab_pipeline *pipelines;
	int pipelines_size;

	pipelines_size = gitlab_get_mr_pipelines(owner, repo, mr_id, &pipelines);
	gitlab_print_pipelines(pipelines, pipelines_size);
	gitlab_free_pipelines(pipelines, pipelines_size);
}
