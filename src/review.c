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

#include <ghcli/color.h>
#include <ghcli/config.h>
#include <ghcli/curl.h>
#include <ghcli/forges.h>
#include <ghcli/github/config.h>
#include <ghcli/json_util.h>
#include <ghcli/review.h>

#include <pdjson/pdjson.h>

#include <limits.h>

void
ghcli_review_print_review_table(
	ghcli_pr_review *headers,
	size_t headers_size)
{
	for (size_t i = 0; i < headers_size; ++i) {
		if (headers[i].state) {
			printf("   %s%s%s - %s - %s%s%s\n",
			       ghcli_setbold(), headers[i].author, ghcli_resetbold(),
			       headers[i].date,
			       ghcli_state_color_str(headers[i].state),
			       headers[i].state,
			       ghcli_resetcolor());
		} else {
			printf("   %s%s%s - %s\n",
			       ghcli_setbold(), headers[i].author, ghcli_resetbold(),
			       headers[i].date);
		}

		pretty_print(headers[i].body, 9, 80, stdout);

		ghcli_review_print_comments(
			headers[i].comments,
			headers[i].comments_size);

		putchar('\n');
	}
}

void
ghcli_review_print_comments(
	ghcli_pr_review_comment *comments,
	size_t                   comments_size)
{
	for (size_t i = 0; i < comments_size; ++i) {
		putchar('\n');
		printf("         PATH : %s\n"
		       "         DIFF :\n",
		       comments[i].path);

		pretty_print(comments[i].diff, 20, INT_MAX, stdout);
		putchar('\n');
		pretty_print(comments[i].body, 16, 80, stdout);
	}
}

void
ghcli_review_reviews_free(ghcli_pr_review *it, size_t size)
{
	if (!it)
		return;

	for (size_t i = 0; i < size; ++i) {
		free(it[i].author);
		free(it[i].date);
		free(it[i].state);
		free(it[i].body);
		free(it[i].id);
	}

	ghcli_review_comments_free(it->comments, it->comments_size);

	free(it);
}

void
ghcli_review_comments_free(ghcli_pr_review_comment *it, size_t size)
{
	if (!it)
		return;

	for (size_t i = 0; i < size; ++i) {
		free(it[i].id);
		free(it[i].author);
		free(it[i].date);
		free(it[i].diff);
		free(it[i].path);
		free(it[i].body);
	}

	free(it);
}

size_t ghcli_review_get_reviews(
	const char       *owner,
	const char       *repo,
	int               pr,
	ghcli_pr_review **out)
{
	return ghcli_forge()->get_reviews(owner, repo, pr, out);
}
