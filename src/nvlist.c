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

#include <gcli/gcli.h>

#include <gcli/nvlist.h>

#include <stdlib.h>
#include <string.h>

int
gcli_nvlist_init(gcli_nvlist *list)
{
	TAILQ_INIT(list);

	return 0;
}

int
gcli_nvlist_free(gcli_nvlist *list)
{
	gcli_nvpair *p1, *p2;

	p1 = TAILQ_FIRST(list);
	while (p1 != NULL) {
		p2 = TAILQ_NEXT(p1, next);

		free(p1->key);
		free(p1->value);
		free(p1);

		p1 = p2;
	}

	TAILQ_INIT(list);

	return 0;
}

int
gcli_nvlist_append(gcli_nvlist *list, char *key, char *value)
{
	/* TODO: handle the case where a pair with an already existing
	 * key is inserted. */

	gcli_nvpair *pair = calloc(1, sizeof(*pair));
	if (pair == NULL)
		return -1;

	pair->key = key;
	pair->value = value;

	TAILQ_INSERT_TAIL(list, pair, next);

	return 0;
}

char const *
gcli_nvlist_find(gcli_nvlist const *list, char const *key)
{
	gcli_nvpair const *pair;

	TAILQ_FOREACH(pair, list,next) {
		if (strcmp(pair->key, key) == 0)
			return pair->value;
	}
	return NULL;
}

char const *
gcli_nvlist_find_or(gcli_nvlist const *list, char const *const key,
                    char const *const alternative)
{
	char const *const result = gcli_nvlist_find(list, key);
	if (result)
		return result;
	else
		return alternative;
}
