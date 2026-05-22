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

/* Watch a pipeline executing in real-time */

#include <gcli/cmd/cmd.h>
#include <gcli/cmd/cmdconfig.h>
#include <gcli/cmd/pipelines.h>
#include <gcli/forges.h>
#include <gcli/gcli.h>
#include <gcli/pipelines.h>

#include <strings.h>
#include <unistd.h>

static bool
finished(struct gcli_pipeline const *const p)
{
	struct gcli_forge_descriptor const *fd =
		gcli_forge(g_clictx);

	char const *const noconcl_done_words[] =
		{ "failed", "success", "canceled", "skipped" };

	if (p->status == NULL)
		return false;

	/* no conclusion? then look at the status */
	if (fd->pipeline_quirks & GCLI_PIPELINE_QUIRKS_NOCONCLUSION) {
		for (size_t i = 0; i < ARRAY_SIZE(noconcl_done_words); ++i) {
			if (strcasecmp(noconcl_done_words[i], p->status) == 0)
				return true;
		}

		return false;;

	} else {
		return strcasecmp("completed", p->status) == 0;
	}
}

int
gcli_cmd_watch_pipeline(struct gcli_path const *const path, int delay)
{
	int rc = 0;
	struct gcli_pipeline pipeline = {0};
	struct gcli_job_list jobs = {0};
	size_t lines = 0, room_needed;
	bool done = false;

	if (delay <= 0)
		delay = gcli_config_get_monitor_delay(g_clictx);

	for (;;) {
		/* fetch the job list */
		rc = gcli_get_pipeline_jobs(g_clictx, path, -1, &jobs);
		if (rc < 0)
			return rc;

		/* make enough room for the table */
		room_needed = (jobs.jobs_size - lines) + 1;

		if (room_needed > lines) {
			for (size_t i = 0; i < (room_needed - lines); ++i)
				printf("\n");
		}

		lines = jobs.jobs_size + 1;

		/* move cursor up, clear downwards */
		printf("\033[%zuA\033[J", lines);
		fflush(stdout);

		gcli_print_jobs(&jobs);
		gcli_free_jobs(&jobs);

		/* query pipeline status */
		rc = gcli_get_pipeline(g_clictx, path, &pipeline);
		if (rc < 0)
			return rc;

		done = finished(&pipeline);
		gcli_pipeline_free(&pipeline);

		if (done)
			break;

		sleep(delay);
	}


	return rc;
}
