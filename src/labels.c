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
#include <gcli/forges.h>
#include <gcli/labels.h>

size_t
gcli_get_labels(
	const char   *owner,
	const char   *reponame,
	int           max,
	gcli_label **out)
{
	return gcli_forge()->get_labels(owner, reponame, max, out);
}

void
gcli_free_labels(gcli_label *labels, size_t labels_size)
{
	for (size_t i = 0; i < labels_size; ++i) {
		free(labels[i].name);
		free(labels[i].description);
	}
	free(labels);
}

void
gcli_print_labels(const gcli_label *labels, size_t labels_size)
{
	printf("%10.10s %-15.15s %s\n", "ID", "NAME", "DESCRIPTION");

	for (size_t i = 0; i < labels_size; ++i) {
		printf(
			"%10.10ld %s%-15.15s%s %s\n",
			labels[i].id,
			gcli_setcolor256(labels[i].color), labels[i].name, gcli_resetcolor(),
			labels[i].description);
	}
}

void
gcli_create_label(const char *owner, const char *repo, gcli_label *label)
{
	gcli_forge()->create_label(owner, repo, label);
}

void
gcli_delete_label(const char *owner, const char *repo, const char *label)
{
	gcli_forge()->delete_label(owner, repo, label);
}
