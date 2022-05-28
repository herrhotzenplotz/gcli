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

#ifndef GITLAB_PIPELINES_H
#define GITLAB_PIPELINES_H

typedef struct gitlab_pipeline gitlab_pipeline;
typedef struct gitlab_job      gitlab_job;

struct gitlab_pipeline {
	long	 id;
	char	*status;
	char    *created_at;
	char    *updated_at;
	char    *ref;
	char    *sha;
	char    *source;
};

struct gitlab_job {
	long	 id;
	char	*status;
	char	*stage;
	char	*name;
	char	*ref;
	char	*created_at;
	char	*started_at;
	char	*finished_at;
	double	 duration;
	char	*runner_name;
	char	*runner_description;
};

int gitlab_get_pipelines(
	const char		 *owner,
	const char		 *repo,
	int				  max,
	gitlab_pipeline **out);

void gitlab_print_pipelines(
	gitlab_pipeline *pipelines,
	int              pipelines_size);

void gitlab_free_pipelines(
	gitlab_pipeline *pipelines,
	int              pipelines_size);

void gitlab_pipelines(
	const char *owner,
	const char *repo,
	int         count);

void gitlab_pipeline_jobs(
	const char *owner,
	const char *repo,
	long        pipeline,
	int         count);

int gitlab_get_pipeline_jobs(
	const char	 *owner,
	const char	 *repo,
	long		  pipeline,
	int			  count,
	gitlab_job	**jobs);

void gitlab_print_jobs(
	gitlab_job	*jobs,
	int			 jobs_size);

void gitlab_free_jobs(
	gitlab_job	*jobs,
	int			 jobs_size);

#endif /* GITLAB_PIPELINES_H */
