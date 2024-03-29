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

#ifndef COLOR_H
#define COLOR_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gcli/gcli.h>

#include <stdint.h>

#include <sn/sn.h>

#define GCLI_256COLOR_DONE 0x3F0FAF00
#define GCLI_256COLOR_OPEN 0x04FF0100

enum {
	GCLI_COLOR_BLACK,
	GCLI_COLOR_RED,
	GCLI_COLOR_GREEN,
	GCLI_COLOR_YELLOW,
	GCLI_COLOR_BLUE,
	GCLI_COLOR_MAGENTA,
	GCLI_COLOR_CYAN,
	GCLI_COLOR_WHITE,
	GCLI_COLOR_DEFAULT,
};

char const *gcli_setcolour256(uint32_t colourcode);
char const *gcli_resetcolour(void);
char const *gcli_setcolour(int colour);
char const *gcli_state_colour_sv(sn_sv const state);
char const *gcli_state_colour_str(char const *it);
char const *gcli_setbold(void);
char const *gcli_resetbold(void);

#endif /* COLOR_H */
