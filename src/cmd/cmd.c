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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gcli/cmd/cmd.h>
#include <gcli/cmd/cmdconfig.h>
#include <gcli/cmd/colour.h>
#include <gcli/port/string.h>
#include <gcli/port/util.h>
#include <gcli/repos.h>

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include <curl/curl.h>

#ifdef HAVE_LIBLOWDOWN
#include <sys/queue.h>

#include <locale.h>
#include <lowdown.h>
#endif

#if defined(HAVE_LIBREADLINE) && !defined(HAVE_LIBEDIT)
#define USE_READLINE 1
#include <readline/readline.h>
#endif

#if defined(HAVE_LIBEDIT)
#define USE_LIBEDIT 1
#include <histedit.h>
#endif

void
copyright(void)
{
	fprintf(
		stderr,
		"Copyright Nico Sonack <nsonack@herrhotzenplotz.de> and contributors.\n");
}

void
version(void)
{
	fprintf(stderr, PACKAGE_STRING" ("HOSTOS")\n");
}

void
longversion(void)
{
	version();
	fprintf(stderr, "Using %s\n", curl_version());
	fprintf(stderr, "Using vendored pdjson library\n");
#ifdef USE_READLINE
	fprintf(stderr, "Using readline version %d.%d\n", RL_VERSION_MAJOR, RL_VERSION_MINOR);
#endif /* USE_READLINE */
#ifdef USE_LIBEDIT
	fprintf(stderr, "Using libedit version %d.%d\n", LIBEDIT_MAJOR, LIBEDIT_MINOR);
#endif /* USE_LIBEDIT */
#ifdef HAVE_LIBLOWDOWN
	fprintf(stderr, "Using liblowdown\n");
#endif /* HAVE_LIBLOWDOWN */
	fprintf(stderr, "\n");
	fprintf(stderr, "Project website: "PACKAGE_URL"\n");
	fprintf(stderr, "Bug reports: "PACKAGE_BUGREPORT"\n");
}

bool
parse_forge_path_arg(int *argc, char ***argv, struct gcli_path *path)
{
	char const *arg, *colon, *slash;
	char *account;

	if (*argc < 2)
		return false;

	arg = (*argv)[1];

	if (arg[0] == '-')
		return false;

	colon = strchr(arg, ':');

	if (colon == NULL) {
		/* Bare account name: look up in config to disambiguate from an
		 * owner name. */
		account = strdup(arg);
		if (gcli_config_find_by_key(g_clictx, account, "forge-type") == NULL) {
			free(account);
			return false;
		}
		gcli_config_set_override_default_account(g_clictx, account);
	} else {
		/* prefix:owner/repo form */
		if (colon == arg)
			return false;

		slash = strrchr(colon + 1, '/');
		if (slash == NULL || slash == colon + 1 || slash[1] == '\0')
			return false;

		if (strncmp(arg, "gh:", 3) == 0) {
			gcli_config_set_override_forgetype(g_clictx, GCLI_FORGE_GITHUB);
		} else if (strncmp(arg, "gl:", 3) == 0) {
			gcli_config_set_override_forgetype(g_clictx, GCLI_FORGE_GITLAB);
		} else if (strncmp(arg, "cb:", 3) == 0) {
			gcli_config_set_override_forgetype(g_clictx, GCLI_FORGE_GITEA);
		} else {
			account = gcli_strndup(arg, (size_t)(colon - arg));
			if (gcli_config_find_by_key(g_clictx, account, "forge-type") == NULL) {
				free(account);
				return false;
			}
			gcli_config_set_override_default_account(g_clictx, account);
		}

		path->as_default.owner =
			gcli_strndup(colon + 1, (size_t)(slash - (colon + 1)));
		path->as_default.repo = strdup(slash + 1);
	}

	/* Consume argv[1]: slide the base pointer forward, keeping argv[0]
	 * (the subcommand name) visible at the new argv[0] position. */
	(*argv)[1] = (*argv)[0];
	(*argv)++;
	(*argc)--;

	return true;
}

void
check_owner_and_repo(char **owner, char **repo)
{
	/* HACK */
	if (gcli_config_get_forge_type(g_clictx) == GCLI_FORGE_BUGZILLA)
		return;

	/* If no remote was specified, try to autodetect */
	if ((*owner == NULL) != (*repo == NULL))
		errx(1, "gcli: error: missing either explicit owner or repo");

	if (*owner == NULL) {
		int rc = gcli_config_get_repo(g_clictx, owner, repo);
		if (rc < 0)
			errx(1, "gcli: error: failed to derive owner/repo combination");
	}
}

void
check_path(struct gcli_path *path)
{
	/* Two special cases for Bugzilla support:
	 *
	 * When no ID was specified with bugzilla we only have a combination of
	 * product/component. in this case we force the path kind to BUGZILLA.
	 *
	 * The other case is a (possibly) missing product and component but an
	 * ID was set. In this case we change the path kind to GCLI_PATH_ID.
	 * We don't ignore product/component because that would be incorrect
	 * and/or leak memory.
	 *
	 * For reasons of human error the juggling below is done such that if
	 * someone by accident breaks the ABI of gcli_path this doesn't fall
	 * apart. */
	if (gcli_config_get_forge_type(g_clictx) == GCLI_FORGE_BUGZILLA &&
	    path->kind == GCLI_PATH_DEFAULT) {

		/* first case */
		if (path->as_default.id == 0) {
			char *const product = path->as_default.owner;
			char *const component = path->as_default.repo;

			path->kind = GCLI_PATH_BUGZILLA;
			path->as_bugzilla.product = product;
			path->as_bugzilla.component = component;

			return; /* no more checking required */
		}

		/* second case */
		if (path->as_default.id != 0
		    && path->as_default.owner == NULL
		    && path->as_default.repo == NULL)
		{
			 gcli_id const id = path->as_default.id;
			 path->kind = GCLI_PATH_ID;
			 path->as_id = id;

			 return;
		}
	}

	check_owner_and_repo(
		&path->as_default.owner,
		&path->as_default.repo);
}

/* Parses (and updates) the given argument list into two seperate lists:
 *
 *   --add    -> add_labels
 *   --remove -> remove_labels
 */
void
parse_labels_options(int *argc, char ***argv,
                     const char ***_add_labels,    size_t *_add_labels_size,
                     const char ***_remove_labels, size_t *_remove_labels_size)
{
	const char **add_labels = NULL, **remove_labels = NULL;
	size_t       add_labels_size = 0, remove_labels_size = 0;

	/* Collect add/delete labels */
	while (*argc >= 3) {
		char const *const action = (*argv)[1];
		char const *const name = (*argv)[2];

		if (strcmp(action, "add") == 0) {
			add_labels = realloc(
				add_labels,
				(add_labels_size + 1) * sizeof(*add_labels));
			add_labels[add_labels_size++] = name;
		} else if (strcmp(action, "remove") == 0) {
			remove_labels = realloc(
				remove_labels,
				(remove_labels_size + 1) * sizeof(*remove_labels));
			remove_labels[remove_labels_size++] = name;
		} else {
			break;
		}

		*argc -= 2;
		*argv += 2;
	}

	*_add_labels      = add_labels;
	*_add_labels_size = add_labels_size;

	*_remove_labels      = remove_labels;
	*_remove_labels_size = remove_labels_size;
}

/* delete the repo (and ask for confirmation)
 *
 * NOTE: this procedure is here because it is used by both the forks
 * and repo subcommand. Ideally it should be moved into the 'repos'
 * code but I don't wanna make it exported from there. */
void
delete_repo(bool always_yes, struct gcli_path const *const path)
{
	bool delete = false;

	if (!always_yes) {
		delete = gcli_yesno("Are you sure you want to delete the repo?");
	} else {
		delete = true;
	}

	if (!delete)
		errx(1, "gcli: Operation aborted");

	if (gcli_repo_delete(g_clictx, path) < 0)
		errx(1, "gcli: error: failed to delete repo");
}

#ifdef HAVE_LIBLOWDOWN
static void
gcli_render_markdown(char const *input, int indent, int maxlinelen, FILE *stream)
{
	size_t input_size;
	struct lowdown_buf *out;
	struct lowdown_doc *doc;
	struct lowdown_node *n;
	struct lowdown_opts opts = {0};
	void *rndr;

	input_size = strlen(input);

	if (setlocale(LC_CTYPE, "en_US.UTF-8") == NULL)
		err(1, NULL);

	opts.feat |= LOWDOWN_FENCED|LOWDOWN_TASKLIST|LOWDOWN_TABLES;
	if (!gcli_config_have_colours(g_clictx))
		opts.oflags |= (LOWDOWN_TERM_NOANSI|LOWDOWN_TERM_NOCOLOUR);

	/* Lowdown 1.4.0 broke the api in a minor version update. Work around
	 * this by checking versions.
	 *
	 * See: https://github.com/kristapsdz/lowdown/issues/148 and
	 *      https://github.com/kristapsdz/lowdown/releases/tag/VERSION_1_4_0 */
#if (LIBLOWDOWN_MAJOR == 1 && LIBLOWDOWN_MINOR >= 4) || LIBLOWDOWN_MAJOR >= 2
	opts.term.vmargin = 1;
	opts.term.hmargin = indent; /* Not only did the minor version break the API but also behaviour ... */
	opts.term.cols = maxlinelen;
#else
	opts.vmargin = 1;
	opts.hmargin = indent - 4; /* somehow there's always 4 spaces being emitted by lowdown */
	opts.cols = maxlinelen;
#endif

	if ((doc = lowdown_doc_new(&opts)) == NULL)
		err(1, NULL);

	if ((n = lowdown_doc_parse(doc, NULL, input, input_size, NULL)) == NULL)
		err(1, NULL);

	if ((out = lowdown_buf_new(256)) == NULL)
		err(1, NULL);

	if ((rndr = lowdown_term_new(&opts)) == NULL)
		err(1, NULL);

	if (!lowdown_term_rndr(out, rndr, n))
		err(1, NULL);

	fwrite(out->data, 1, out->size, stream);

	lowdown_term_free(rndr);
	lowdown_buf_free(out);
	lowdown_node_free(n);
	lowdown_doc_free(doc);
}
#endif

static int
word_length(const char *x)
{
	int l = 0;

	while (*x && !isspace(*x++))
		l++;
	return l;
}

void
gcli_pretty_print(const char *input, int indent, int maxlinelen, FILE *out)
{
	const char *it = input;

	if (!it)
		return;

#ifdef HAVE_LIBLOWDOWN
	if (gcli_config_render_markdown(g_clictx)) {
		gcli_render_markdown(input, indent, maxlinelen, out);
		return;
	}
#endif

	while (*it) {
		int linelength = indent;
		fprintf(out, "%*.*s", indent, indent, "");

		do {
			int w = word_length(it) + 1;

			if (it[w - 1] == '\n') {
				fprintf(out, "%.*s", w - 1, it);
				it += w;
				break;
			} else if (it[w - 1] == '\0') {
				w -= 1;
			}

			fprintf(out, "%.*s", w, it);
			it += w;
			linelength += w;

		} while (*it && (linelength < maxlinelen));

		fputc('\n', out);
	}
}

void
gcli_pretty_print_diff(char const *const input, int indent)
{
	char const *hd = input;

	for (;;) {
		char const *eol;
		char const *start_colour, *end_colour;
		size_t linelen;

		if (hd == NULL || *hd == '\0')
			return;

		eol = strchr(hd, '\n');
		if (eol == NULL)
			eol = hd + strlen(hd);

		linelen = eol - hd;
		end_colour = gcli_resetcolour();
		if (*hd == '+')
			start_colour = gcli_setcolour(GCLI_COLOR_GREEN);
		else if (*hd == '-')
			start_colour = gcli_setcolour(GCLI_COLOR_RED);
		else
			start_colour = "";

		printf("%*.*s%s%.*s%s\n", indent, indent, "", start_colour,
		       (int)linelen, hd, end_colour);
		hd = eol + 1;
	}
}

bool
gcli_cmd_should_do_always_yes(void)
{
	return !isatty(STDIN_FILENO);
}

void
gcli_cmd_save_message(char const *const message)
{
	FILE *f = fopen("gcli_message", "w");
	if (!f) {
		fprintf(stderr, "gcli: warning: failed to open 'gcli_message' "
		        "for write, cannot save message\n");
		return;
	}

	fputs(message, f);
	fclose(f);

	fprintf(stderr, "gcli: Message was saved in 'gcli_message'. "
	                "Re-run the command to recall it.\n");
}

bool
gcli_cmd_can_recall_message(void)
{
	return !access("gcli_message", R_OK);
}

char *
gcli_cmd_recall_message(void)
{
	char *result = NULL;
	int rc = 0;

	/* read */
	rc = gcli_read_file("gcli_message", &result);
	if (rc < 0)
		return NULL;

	/* delete old message file */
	rc = unlink("gcli_message");
	if (rc < 0) {
		fprintf(stderr, "gcli: warning: cannot delete gcli_message: %s\n",
		        strerror(errno));
	}

	return result;
}

void
gcli_cmd_recall_message_interactive(char **out)
{
	/* recall an old message if needed, skip if there is a template */
	if (*out == NULL && gcli_cmd_can_recall_message()) {
		if (gcli_yesno("Recall previously saved message?"))
			*out = gcli_cmd_recall_message();
	}
}
