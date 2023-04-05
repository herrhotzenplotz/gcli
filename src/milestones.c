/*
 * Copyright 2023 Nico Sonack <nsonack@herrhotzenplotz.de>
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
#include <gcli/milestones.h>
#include <gcli/table.h>

int
gcli_get_milestones(char const *const owner,
                    char const *const repo,
                    int const max,
                    gcli_milestone_list *const out)
{
	return gcli_forge()->get_milestones(owner, repo, max, out);
}

int
gcli_get_milestone(char const *owner,
                   char const *repo,
                   int const milestone,
                   gcli_milestone *const out)
{
	return gcli_forge()->get_milestone(owner, repo, milestone, out);
}

int
gcli_create_milestone(struct gcli_milestone_create_args const *args)
{
	return gcli_forge()->create_milestone(args);
}

int
gcli_delete_milestone(char const *const owner,
                      char const *const repo,
                      int const milestone)
{
	return gcli_forge()->delete_milestone(owner, repo, milestone);
}

void
gcli_print_milestones(gcli_milestone_list const *const list,
                      int max)
{
	size_t n;
	gcli_tbl tbl;
	gcli_tblcoldef cols[] = {
		{ .name = "ID",      .type = GCLI_TBLCOLTYPE_INT,    .flags = GCLI_TBLCOL_JUSTIFYR },
		{ .name = "STATE",   .type = GCLI_TBLCOLTYPE_STRING, .flags = GCLI_TBLCOL_STATECOLOURED },
		{ .name = "CREATED", .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
		{ .name = "TITLE",   .type = GCLI_TBLCOLTYPE_STRING, .flags = 0 },
	};

	if (!list->milestones_size) {
		puts("No milestones");
		return;
	}

	tbl = gcli_tbl_begin(cols, ARRAY_SIZE(cols));
	if (!tbl)
		errx(1, "error: could not init table printer");

	if (max < 0 || (size_t)(max) > list->milestones_size)
		n = list->milestones_size;
	else
		n = max;

	for (size_t i = 0; i < n; ++i) {
		gcli_tbl_add_row(tbl,
		                 list->milestones[i].id,
		                 list->milestones[i].state,
		                 list->milestones[i].created_at,
		                 list->milestones[i].title);
	}

	gcli_tbl_end(tbl);
}

void
gcli_print_milestone(gcli_milestone const *const milestone)
{
	gcli_dict dict;
	uint32_t const quirks = gcli_forge()->milestone_quirks;

	dict = gcli_dict_begin();
	gcli_dict_add(dict,        "ID", 0, 0, "%d", milestone->id);
	gcli_dict_add_string(dict, "TITLE", 0, 0, milestone->title);
	gcli_dict_add_string(dict, "STATE", GCLI_TBLCOL_STATECOLOURED, 0, milestone->state);
	gcli_dict_add_string(dict, "CREATED", 0, 0, milestone->created_at);
	gcli_dict_add_string(dict, "UPDATED", 0, 0, milestone->created_at);

	if ((quirks & GCLI_MILESTONE_QUIRKS_DUEDATE) == 0)
		gcli_dict_add_string(dict, "DUE", 0, 0, milestone->due_date);

	if ((quirks & GCLI_MILESTONE_QUIRKS_EXPIRED) == 0)
		gcli_dict_add_string(dict, "EXPIRED", 0, 0, sn_bool_yesno(milestone->expired));

	if ((quirks & GCLI_MILESTONE_QUIRKS_NISSUES) == 0) {
		gcli_dict_add(dict, "OPEN ISSUES", 0, 0, "%d", milestone->open_issues);
		gcli_dict_add(dict, "CLOSED ISSUES", 0, 0, "%d", milestone->closed_issues);
	}

	gcli_dict_end(dict);

	if (milestone->description && strlen(milestone->description)) {
		printf("\nDESCRIPTION:\n");
		pretty_print(milestone->description, 4, 80, stdout);
	}
}

void
gcli_free_milestone(gcli_milestone *const it)
{
	free(it->title);
	it->title = NULL;
	free(it->state);
	it->state = NULL;
	free(it->created_at);
	it->created_at = NULL;

	free(it->description);
	it->description = NULL;
	free(it->updated_at);
	it->updated_at = NULL;
	free(it->due_date);
	it->due_date = NULL;
}

void
gcli_free_milestones(gcli_milestone_list *const it)
{
	for (size_t i = 0; i < it->milestones_size; ++i)
		gcli_free_milestone(&it->milestones[i]);

	free(it->milestones);
	it->milestones = NULL;
	it->milestones_size = 0;
}

int
gcli_milestone_get_issues(char const *const owner,
                          char const *const repo,
                          int const milestone,
                          gcli_issue_list *const out)
{
	return gcli_forge()->get_milestone_issues(owner, repo, milestone, out);
}

int
gcli_milestone_set_duedate(char const *const owner,
                           char const *const repo,
                           int const milestone,
                           char const *const date)
{
	return gcli_forge()->milestone_set_duedate(owner, repo, milestone, date);
}
