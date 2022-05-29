/*
 * Copyright 2022 Nico Sonack <nsonack@outlook.com>
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

#include <stdlib.h>

static struct {
	uint32_t code;
	char *sequence;
} color_table[1024];
static size_t color_table_size;

static void
clean_color_table(void)
{
	for (size_t i = 0; i < color_table_size; ++i)
		free(color_table[i].sequence);
}

static char *
color_cache_lookup(uint32_t code)
{
	for (size_t i = 0; i < color_table_size; ++i) {
		if (color_table[i].code == code)
			return color_table[i].sequence;
	}
	return NULL;
}

static void
color_cache_insert(uint32_t code, char *sequence)
{
	color_table[color_table_size].code     = code;
	color_table[color_table_size].sequence = sequence;
	color_table_size++;
}

const char *
ghcli_setcolor256(uint32_t code)
{
	char *result = NULL;

	if (!ghcli_config_have_colors())
		return "";

	if (color_table_size == 0)
		atexit(clean_color_table);

	result = color_cache_lookup(code);
	if (result)
		return result;

	/* TODO: This is inherently screwed */
	result = sn_asprintf("\033[38;2;%02d;%02d;%02dm",
				 (code & 0xFF000000) >> 24,
				 (code & 0x00FF0000) >> 16,
				 (code & 0x0000FF00) >>  8);

	color_cache_insert(code, result);

	return result;
}

const char *
ghcli_resetcolor(void)
{
	if (!ghcli_config_have_colors())
		return "";

	return "\033[m";
}

const char *
ghcli_setcolor(int code)
{
	if (!ghcli_config_have_colors())
		return "";

	switch (code) {
	case GHCLI_COLOR_BLACK:   return "\033[30m";
	case GHCLI_COLOR_RED:     return "\033[31m";
	case GHCLI_COLOR_GREEN:   return "\033[32m";
	case GHCLI_COLOR_YELLOW:  return "\033[33m";
	case GHCLI_COLOR_BLUE:    return "\033[34m";
	case GHCLI_COLOR_MAGENTA: return "\033[35m";
	case GHCLI_COLOR_CYAN:    return "\033[36m";
	case GHCLI_COLOR_WHITE:   return "\033[37m";
	case GHCLI_COLOR_DEFAULT: return "\033[39m";
	default:
		sn_notreached;
	}
	return NULL;
}

const char *
ghcli_setbold(void)
{
	if (!ghcli_config_have_colors())
		return "";
	else
		return "\033[1m";
}

const char *
ghcli_resetbold(void)
{
	if (!ghcli_config_have_colors())
		return "";
	else
		return "\033[22m";
}

const char *
ghcli_state_color_str(const char *it)
{
	if (it)
		return ghcli_state_color_sv(SV((char *)it));
	else
		return "";
}

/* TODO: Probably a hash table would be more suitable */
static const struct { const char *name; int code; }
	state_color_table[] =
{
	{ .name = "open",      .code = GHCLI_COLOR_GREEN   },
	{ .name = "success",   .code = GHCLI_COLOR_GREEN   },
	{ .name = "APPROVED",  .code = GHCLI_COLOR_GREEN   },
	{ .name = "merged",    .code = GHCLI_COLOR_MAGENTA },
	{ .name = "closed",    .code = GHCLI_COLOR_RED     },
	{ .name = "failed",    .code = GHCLI_COLOR_RED     },
	{ .name = "canceled",  .code = GHCLI_COLOR_RED     }, /* orthography has left the channel */
	{ .name = "failure",   .code = GHCLI_COLOR_RED     },
	{ .name = "running",   .code = GHCLI_COLOR_BLUE    },
	{ .name = "COMMENTED", .code = GHCLI_COLOR_BLUE    },
};

const char *
ghcli_state_color_sv(sn_sv state)
{
	if (!sn_sv_null(state)) {
		for (size_t i = 0; i < ARRAY_SIZE(state_color_table); ++i) {
			if (sn_sv_has_prefix(state, state_color_table[i].name))
				return ghcli_setcolor(state_color_table[i].code);
		}
	}

	return ghcli_setcolor(GHCLI_COLOR_DEFAULT);
}
