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

#include <gcli/cmd/actions.h>
#include <gcli/cmd/cmd.h>
#include <gcli/cmd/interactive.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct gcli_cmd_action const *
find_action(struct gcli_cmd_actions const *const actions,
            char const *const action_name)
{
	struct gcli_cmd_action const *a = NULL;

	for (a = actions->defs; a->name != NULL; a += 1) {
		if (strcmp(a->name, action_name) == 0) {
			return a;
		}
	}

	return NULL;
}

int
gcli_cmd_actions_handle(struct gcli_cmd_actions const *const actions,
                        struct gcli_path const *const path,
                        int *argc, char ***argv)
{
	void *item = NULL;
	int rc = 0;

	if (*argc < 1) {
		fprintf(stderr, "gcli: error: missing action\n");
		return GCLI_EX_USAGE;
	}

	/* check until we don't have any more remaining arguments */
	for (;;) {

		/* fetch the action name */
		char *const action_name = (*argv)[0];

		/* look for an action definition */
		struct gcli_cmd_action const *const action = find_action(
			actions, action_name);

		if (action == NULL) {
			fprintf(stderr, "gcli: error: unknown action '%s'\n",
			        action_name);

			rc = GCLI_EX_USAGE;
			break;
		}

		/* check whether we need to fetch the item */
		if (action->needs_item && item == NULL) {
			item = calloc(1, actions->item_size);
			if (item == NULL) {
				err(1, "calloc failed");
			}

			rc = actions->fetch_item(g_clictx, path, item);
			if (rc < 0) {
				fprintf(stderr, "gcli: error: failed to fetch: %s\n",
				        gcli_get_error(g_clictx));

				rc = GCLI_EX_DATAERR;
				break;
			}
		}

		/* handle the action */
		rc = action->handler(path, item, argc, argv);
		if (rc < 0) {
			fprintf(stderr, "gcli: action %s failed\n", action_name);
			break;
		}

		shift(argc, argv);

		if (*argc == 0)
			break;

		fputc('\n', stdout);
	}

	if (item) {
		actions->free_item(item);
		free(item);
		item = NULL;
	}

	return rc;
}

struct into_pager_args {
	int argc;
	char **argv;
	struct gcli_path const *path;
	void *item;
	struct gcli_cmd_action const *action;
};

static int
into_pager_fn(void *data)
{
	struct into_pager_args *args = data;
	return args->action->handler(
		args->path,
		args->item,
		&args->argc,
		&args->argv);
}

int
gcli_cmd_action_handle(struct gcli_cmd_actions const *actions,
                       struct gcli_path const *path,
                       char *cmd_input)
{
	enum { argv_size = 32 };
	char *_argv[argv_size]; /* storage */
	char *argfront = cmd_input;
	int argc = 0, rc = 0;
	struct gcli_cmd_action const *action = NULL;
	void *item = NULL;

	char **argv = &_argv[0]; /* pointer to storage but type is corrected */

	/* Split arguments by spaces and collect into argc/argv */
	for (;;) {
		char *argnext = strchr(argfront, ' ');

		if (argc == argv_size)
			err(1, "gcli: error: too many arguments");

		argv[argc++] = argfront;

		if (!argnext)
			break;

		*argnext++ = '\0';
		argfront = argnext;
	}

	action = find_action(actions, argv[0]);
	if (action == NULL) {
		fprintf(stderr, "gcli: error: no such action: %s\n", argv[0]);
		return GCLI_EX_USAGE;
	}

	if (action->needs_item) {
		item = calloc(1, actions->item_size);
		if (item == NULL)
			err(1, "calloc");

		rc = actions->fetch_item(g_clictx, path, item);
		if (rc < 0) {
			fprintf(stderr, "gcli: error: failed to fetch item: %s\n",
			        gcli_get_error(g_clictx));
			return GCLI_EX_DATAERR;
		}
	}

	if (action->use_pager) {
		struct into_pager_args args = {
			.argc = argc,
			.argv = argv,
			.path = path,
			.item = item,
			.action = action,
		};

		rc = gcli_cmd_into_pager(into_pager_fn, &args);
	} else {
		rc = action->handler(path, item, &argc, &argv);
	}

	if (item) {
		actions->free_item(item);
		free(item);
		item = NULL;
	}

	return rc;
}
