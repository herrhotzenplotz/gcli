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

#include <gcli/forges.h>
#include <gcli/pipelines.h>

void
gcli_pipeline_free(struct gcli_pipeline *pipeline)
{
	gcli_clear_ptr(&pipeline->status);
	gcli_clear_ptr(&pipeline->ref);
	gcli_clear_ptr(&pipeline->sha);
	gcli_clear_ptr(&pipeline->source);
	gcli_clear_ptr(&pipeline->web_url);
}

void
gcli_pipelines_free(struct gcli_pipeline_list *const list)
{
	for (size_t i = 0; i < list->pipelines_size; ++i) {
		gcli_pipeline_free(&list->pipelines[i]);
	}

	gcli_clear_ptr(&list->pipelines);
	list->pipelines_size = 0;
}

void
gcli_free_job(struct gcli_job *const job)
{
	gcli_clear_ptr(&job->status);
	gcli_clear_ptr(&job->stage);
	gcli_clear_ptr(&job->name);
	gcli_clear_ptr(&job->ref);
	gcli_clear_ptr(&job->runner_name);
	gcli_clear_ptr(&job->runner_description);
	gcli_clear_ptr(&job->web_url);
}

void
gcli_free_jobs(struct gcli_job_list *list)
{
	for (size_t i = 0; i < list->jobs_size; ++i)
		gcli_free_job(&list->jobs[i]);

	gcli_clear_ptr(&list->jobs);
	list->jobs_size = 0;
}

int
gcli_get_pipelines(struct gcli_ctx *ctx,
                   struct gcli_path const *repo_path,
                   struct gcli_pipelines_fetch_details const *details,
                   struct gcli_pipeline_list *out)
{
	gcli_null_check_call(get_pipelines, ctx, repo_path, details, out);
}

int
gcli_get_pipeline(struct gcli_ctx *ctx,
                  struct gcli_path const *pipeline_path,
                  struct gcli_pipeline *out)
{
	gcli_null_check_call(get_pipeline, ctx, pipeline_path, out);
}
