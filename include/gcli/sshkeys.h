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

#ifndef GCLI_SSHKEYS_H
#define GCLI_SSHKEYS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stddef.h>

struct gcli_sshkey {
	int id;
	char *title;
	char *key;
	char *created_at;
};

struct gcli_sshkey_list {
	struct gcli_sshkey *keys;
	size_t keys_size;
};

typedef struct gcli_sshkey gcli_sshkey;
typedef struct gcli_sshkey_list gcli_sshkey_list;

int gcli_sshkeys_get_keys(gcli_sshkey_list *out);
int gcli_sshkeys_add_key(char const *title,
                         char const *public_key_path,
                         gcli_sshkey *out);
void gcli_sshkeys_print_keys(gcli_sshkey_list const *list);
void gcli_sshkeys_free_keys(gcli_sshkey_list *list);

#endif /* GCLI_SSHKEYS_H */
