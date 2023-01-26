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

#include <gcli/colour.h>
#include <gcli/config.h>

#include <stdlib.h>

static struct {
	uint32_t code;
	char *sequence;
} colour_table[1024];
static size_t colour_table_size;

static void
clean_colour_table(void)
{
	for (size_t i = 0; i < colour_table_size; ++i)
		free(colour_table[i].sequence);
}

static char const *
colour_cache_lookup(uint32_t const code)
{
	for (size_t i = 0; i < colour_table_size; ++i) {
		if (colour_table[i].code == code)
			return colour_table[i].sequence;
	}
	return NULL;
}

static void
colour_cache_insert(uint32_t const code, char *sequence)
{
	colour_table[colour_table_size].code     = code;
	colour_table[colour_table_size].sequence = sequence;
	colour_table_size++;
}

char const *
gcli_setcolour256(uint32_t const code)
{
	char       *result    = NULL;
	char const *oldresult = NULL;

	if (!gcli_config_have_colours())
		return "";

	if (colour_table_size == 0)
		atexit(clean_colour_table);

	oldresult = colour_cache_lookup(code);
	if (oldresult)
		return oldresult;

	/* TODO: This is inherently screwed */
	result = sn_asprintf("\033[38;2;%02d;%02d;%02dm",
	                     (code & 0xFF000000) >> 24,
	                     (code & 0x00FF0000) >> 16,
	                     (code & 0x0000FF00) >>  8);

	colour_cache_insert(code, result);

	return result;
}

const char *
gcli_resetcolour(void)
{
	if (!gcli_config_have_colours())
		return "";

	return "\033[m";
}

const char *
gcli_setcolour(int code)
{
	if (!gcli_config_have_colours())
		return "";

	switch (code) {
	case GCLI_COLOR_BLACK:   return "\033[30m";
	case GCLI_COLOR_RED:     return "\033[31m";
	case GCLI_COLOR_GREEN:   return "\033[32m";
	case GCLI_COLOR_YELLOW:  return "\033[33m";
	case GCLI_COLOR_BLUE:    return "\033[34m";
	case GCLI_COLOR_MAGENTA: return "\033[35m";
	case GCLI_COLOR_CYAN:    return "\033[36m";
	case GCLI_COLOR_WHITE:   return "\033[37m";
	case GCLI_COLOR_DEFAULT: return "\033[39m";
	default:
		sn_notreached;
	}
	return NULL;
}

char const *
gcli_setbold(void)
{
	if (!gcli_config_have_colours())
		return "";
	else
		return "\033[1m";
}

char const *
gcli_resetbold(void)
{
	if (!gcli_config_have_colours())
		return "";
	else
		return "\033[22m";
}

char const *
gcli_state_colour_str(char const *it)
{
	if (it)
		return gcli_state_colour_sv(SV((char *)it));
	else
		return "";
}

/* TODO: Probably a hash table would be more suitable */
static const struct { char const *name; int code; }
	state_colour_table[] =
{
	{ .name = "open",      .code = GCLI_COLOR_GREEN   },
	{ .name = "success",   .code = GCLI_COLOR_GREEN   },
	{ .name = "APPROVED",  .code = GCLI_COLOR_GREEN   },
	{ .name = "merged",    .code = GCLI_COLOR_MAGENTA },
	{ .name = "closed",    .code = GCLI_COLOR_RED     },
	{ .name = "failed",    .code = GCLI_COLOR_RED     },
	{ .name = "canceled",  .code = GCLI_COLOR_RED     }, /* orthography has left the channel */
	{ .name = "failure",   .code = GCLI_COLOR_RED     },
	{ .name = "running",   .code = GCLI_COLOR_BLUE    },
	{ .name = "created",   .code = GCLI_COLOR_BLUE    },
	{ .name = "COMMENTED", .code = GCLI_COLOR_BLUE    },
	{ .name = "pending",   .code = GCLI_COLOR_CYAN    },
};

char const *
gcli_state_colour_sv(sn_sv const state)
{
	if (!sn_sv_null(state)) {
		for (size_t i = 0; i < ARRAY_SIZE(state_colour_table); ++i) {
			if (sn_sv_has_prefix(state, state_colour_table[i].name))
				return gcli_setcolour(state_colour_table[i].code);
		}
	}

	return gcli_setcolour(GCLI_COLOR_DEFAULT);
}
