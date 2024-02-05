/*
 * Copyright 2021, 2022 Nico Sonack <nsonack@herrhotzenplotz.de>
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

#include <config.h>

#include <gcli/cmd/cmdconfig.h>
#include <gcli/cmd/gitconfig.h>

#include <gcli/forges.h>
#include <gcli/gitea/config.h>
#include <gcli/github/config.h>
#include <gcli/gitlab/config.h>

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <stdlib.h>
#include <unistd.h>

struct gcli_config_section {
	TAILQ_ENTRY(gcli_config_section) next;

    struct gcli_config_entries entries;

	sn_sv title;
};

struct gcli_config {
	TAILQ_HEAD(gcli_config_sections, gcli_config_section) sections;

	char const *override_default_account;
	char const *override_remote;
	int override_forgetype;
	int colours_disabled;       /* NO_COLOR set or output is not a TTY */
	int force_colours;          /* -c option was given */

	sn_sv  buffer;
	void  *mmap_pointer;
	bool   inited;
};

struct gcli_dotgcli {
	struct gcli_config_entries entries;

	sn_sv  buffer;
	void  *mmap_pointer;
	bool   has_been_searched_for;
	bool   has_been_found;
};

struct cmd_ctx {
	struct gcli_config config;
	struct gcli_dotgcli local_config;
};

static inline struct gcli_dotgcli *
ctx_dotgcli(struct gcli_ctx *ctx)
{
	struct cmd_ctx *cctx = gcli_get_userdata(ctx);
	return &cctx->local_config;
}

static inline struct gcli_config *
ctx_config(struct gcli_ctx *ctx)
{
	struct cmd_ctx *cctx = gcli_get_userdata(ctx);
	return &cctx->config;
}

static bool
should_init_dotgcli(struct gcli_ctx *ctx)
{
	struct gcli_dotgcli *dgcli = ctx_dotgcli(ctx);

	return !dgcli->has_been_searched_for ||
		(dgcli->has_been_searched_for && !dgcli->has_been_found);
}

static char const *
find_dotgcli(void)
{
	char          *curr_dir_path = NULL;
	char          *dotgcli       = NULL;
	DIR           *curr_dir      = NULL;
	struct dirent *ent           = NULL;

	curr_dir_path = getcwd(NULL, 128);
	if (!curr_dir_path)
		err(1, "gcli: getcwd");

	/* Here we are trying to traverse upwards through the directory
	 * tree, searching for a directory called .git.
	 * Starting point is ".".*/
	do {
		curr_dir = opendir(curr_dir_path);
		if (!curr_dir)
			err(1, "gcli: opendir");

		while ((ent = readdir(curr_dir))) {
			if (strcmp(".", ent->d_name) == 0 || strcmp("..", ent->d_name) == 0)
				continue;

			if (strcmp(".gcli", ent->d_name) == 0) {
				size_t len = strlen(curr_dir_path);
				dotgcli = malloc(len + strlen(ent->d_name) + 2);
				memcpy(dotgcli, curr_dir_path, len);
				dotgcli[len] = '/';
				memcpy(dotgcli + len + 1, ent->d_name, strlen(ent->d_name));

				dotgcli[len + 1 + strlen(ent->d_name)] = 0;

				break;
			}
		}


		if (!dotgcli) {
			size_t len = strlen(curr_dir_path);
			char *tmp = malloc(len + sizeof("/.."));

			memcpy(tmp, curr_dir_path, len);
			memcpy(tmp + len, "/..", sizeof("/.."));

			free(curr_dir_path);

			curr_dir_path = realpath(tmp, NULL);
			if (!curr_dir_path)
				err(1, "gcli: realpath at %s", tmp);

			free(tmp);

			if (strcmp("/", curr_dir_path) == 0) {
				free(curr_dir_path);
				closedir(curr_dir);

				// At this point we know for sure that we cannot find
				// a .gcli and thus return a NULL pointer
				return NULL;
			}
		}


		closedir(curr_dir);
	} while (dotgcli == NULL);

	free(curr_dir_path);

	return dotgcli;
}

static void
init_local_config(struct gcli_ctx *ctx)
{
	if (!should_init_dotgcli(ctx)) {
		return;
	}

	char const *path = find_dotgcli();
	struct gcli_dotgcli *dgcli = ctx_dotgcli(ctx);

	if (!path) {
		dgcli->has_been_searched_for = true;
		dgcli->has_been_found = false;
		return;
	}

	dgcli->has_been_searched_for = true;
	dgcli->has_been_found = true;

	int len = sn_mmap_file(path, &dgcli->mmap_pointer);
	if (len < 0)
		err(1, "gcli: unable to open config file");

	dgcli->buffer = sn_sv_from_parts(dgcli->mmap_pointer, len);
	dgcli->buffer = sn_sv_trim_front(dgcli->buffer);

	int curr_line = 1;
	while (dgcli->buffer.length > 0) {
		sn_sv line = sn_sv_chop_until(&dgcli->buffer, '\n');

		line = sn_sv_trim(line);

		if (line.length == 0)
			errx(1, "gcli: %s:%d: Unexpected end of line",
			     path, curr_line);

		// Comments
		if (line.data[0] == '#') {
			dgcli->buffer = sn_sv_trim_front(dgcli->buffer);
			curr_line++;
			continue;
		}

		sn_sv key = sn_sv_chop_until(&line, '=');

		key = sn_sv_trim(key);

		if (key.length == 0)
			errx(1, "gcli: %s:%d: empty key", path, curr_line);

		line.data   += 1;
		line.length -= 1;

		sn_sv value = sn_sv_trim(line);

		struct gcli_config_entry *entry = calloc(sizeof(*entry), 1);
		TAILQ_INSERT_TAIL(&dgcli->entries, entry, next);

	    entry->key = key;
	    entry->value = value;

		dgcli->buffer = sn_sv_trim_front(dgcli->buffer);
		curr_line++;
	}

	free((void *)path);
}

struct config_parser {
	sn_sv buffer;
	int line;
	char const *filename;
};

static void
skip_ws_and_comments(struct config_parser *input)
{
again:
	while (input->buffer.length > 0) {
		switch (input->buffer.data[0]) {
		case '\n':
			input->line++;
			/* fallthrough */
		case ' ':
		case '\t':
		case '\r':
			input->buffer.data   += 1;
			input->buffer.length -= 1;
			break;
		default:
			goto not_whitespace;
		}
	}

	return;

not_whitespace:
	if (input->buffer.data[0] == '#') {
		/* This is a comment */
		sn_sv_chop_until(&input->buffer, '\n');
		goto again;
	}
}

static void
parse_section_entry(struct config_parser *input,
                    struct gcli_config_section *section)
{
	struct gcli_config_entry *entry = calloc(sizeof(*entry), 1);
	TAILQ_INSERT_TAIL(&section->entries, entry, next);

	sn_sv key = sn_sv_chop_until(&input->buffer, '=');

	if (key.length == 0)
		errx(1, "gcli: %s:%d: empty key", input->filename, input->line);

	input->buffer.data   += 1;
	input->buffer.length -= 1;

	sn_sv value = sn_sv_chop_until(&input->buffer, '\n');

	entry->key   = sn_sv_trim(key);
	entry->value = sn_sv_trim(value);
}

static sn_sv
parse_section_title(struct config_parser *input)
{
	size_t len = 0;
	if (input->buffer.length == 0)
		errx(1, "gcli: %s:%d: unexpected end of input in section title",
		     input->filename, input->line);


	while (!isspace(input->buffer.data[len]) && input->buffer.data[len] != '{')
		len++;

	sn_sv title = sn_sv_from_parts(input->buffer.data, len);
	input->buffer.data   += len;
	input->buffer.length -= len;

	skip_ws_and_comments(input);

	if (input->buffer.length == 0)
		errx(1, "gcli: %s:%d: unexpected end of input", input->filename, input->line);

	if (input->buffer.data[0] != '{')
		errx(1, "gcli: %s:%d: expected '{'", input->filename, input->line);

	input->buffer.length -= 1;
	input->buffer.data   += 1;

	skip_ws_and_comments(input);
	return title;
}

static void
parse_config_section(struct gcli_config *cfg,
                     struct config_parser *input)
{
	struct gcli_config_section *section = NULL;

	section = calloc(sizeof(*section), 1);
	TAILQ_INSERT_TAIL(&cfg->sections, section, next);

	section->title = parse_section_title(input);

	section->entries = (struct gcli_config_entries)
		TAILQ_HEAD_INITIALIZER(section->entries);

	while (input->buffer.length > 0 && input->buffer.data[0] != '}') {
		skip_ws_and_comments(input);
		parse_section_entry(input, section);
		skip_ws_and_comments(input);
	}

	if (input->buffer.length == 0)
		errx(1, "gcli: %s:%d: missing '}' before end of file",
		     input->filename, input->line);

	input->buffer.length -= 1;
	input->buffer.data   += 1;
}

static void
parse_config_file(struct gcli_config *cfg,
                  struct config_parser *input)
{
	skip_ws_and_comments(input);

	while (input->buffer.length > 0) {
		parse_config_section(cfg, input);
		skip_ws_and_comments(input);
	}
}

/**
 * Try to load up the local config file if it exists. If we succeed,
 * return 0. Otherwise return -1.
 */
static struct gcli_config *
ensure_config(struct gcli_ctx *ctx)
{
	struct gcli_config *cfg = ctx_config(ctx);
	char *file_path = NULL;
	struct config_parser parser = {0};

	if (cfg->inited)
		return cfg;

	cfg->inited = true;

	file_path = getenv("XDG_CONFIG_PATH");
	if (!file_path) {
		file_path = getenv("HOME");
		if (!file_path) {
			warnx("Neither XDG_CONFIG_PATH nor HOME set in env");
			return cfg;
		}

		/*
		 * Code duplication to avoid leaking pointers */
		file_path = sn_asprintf("%s/.config/gcli/config", file_path);
	} else {
		file_path = sn_asprintf("%s/gcli/config", file_path);
	}

	if (access(file_path, R_OK) < 0) {
		warn("gcli: cannot access config file at %s", file_path);
		return cfg;
	}

	int len = sn_mmap_file(file_path, &cfg->mmap_pointer);
	if (len < 0)
		err(1, "gcli: unable to open config file");

	cfg->buffer = sn_sv_from_parts(cfg->mmap_pointer, len);
	cfg->buffer = sn_sv_trim_front(cfg->buffer);

	parser.buffer   = cfg->buffer;
	parser.line     = 1;
	parser.filename = file_path;

	parse_config_file(cfg, &parser);

	free((void *)file_path);

	return cfg;
}

/** Check input for a value that indicates yes/true */
static int
checkyes(char const *const tmp)
{
	static char const *const yeses[] = { "1", "y", "Y" };

	if (strlen(tmp) == 3) {
		if (tolower(tmp[0]) == 'y' && tolower(tmp[1]) == 'e' &&
		    tolower(tmp[2]) == 's')
			return 1;
	}

	for (size_t i = 0; i < ARRAY_SIZE(yeses); ++i) {
		if (strcmp(yeses[i], tmp) == 0)
			return 1;
	}

	return 0;
}

/* readenv: Read values of environment variables and pre-populate the
 *          config structure. */
static void
readenv(struct gcli_config *cfg)
{
	char *tmp;

	/* A default override account. Can be overridden again by
	 * specifying -a <account-name> */
	if ((tmp = getenv("GCLI_ACCOUNT")))
		cfg->override_default_account = tmp;

	/* NO_COLOR: https://no-color.org/
	 *
	 * Note: the example implementation code on the website is
	 * semantically buggy as it just checks for the variable being set
	 * to ANYTHING. If you set it to 0 to indicate that you want
	 * colours it will still disable colour output. This explicitly
	 * checks the value of the variable if it is set. I purposefully
	 * violate the definition to get expected and sane behaviour. */
	tmp = getenv("NO_COLOR");
	if (tmp && tmp[0] != '\0')
		cfg->colours_disabled = checkyes(tmp);
}

int
gcli_config_init_ctx(struct gcli_ctx *ctx)
{
	struct cmd_ctx *cctx = calloc(sizeof(*cctx), 1);
	gcli_set_userdata(ctx, cctx);

	cctx->config.sections =
		(struct gcli_config_sections)
		TAILQ_HEAD_INITIALIZER(cctx->config.sections);

	cctx->local_config.entries =
		(struct gcli_config_entries)
		TAILQ_HEAD_INITIALIZER(cctx->local_config.entries);

	return 0;
}

int
gcli_config_parse_args(struct gcli_ctx *ctx, int *argc, char ***argv)
{
	/* These are the very first options passed to the gcli command
	 * itself. It is the first ever getopt call we do to parse any
	 * arguments. Only global options that do not alter subcommand
	 * specific behaviour should be accepted here. */

	struct gcli_config *cfg = ctx_config(ctx);

	int ch;
	const struct option options[] = {
		{ .name    = "account",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'a' },
		{ .name    = "remote",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 'r' },
		{ .name    = "colours",
		  .has_arg = no_argument,
		  .flag    = &cfg->colours_disabled,
		  .val     = 0 },
		{ .name    = "type",
		  .has_arg = required_argument,
		  .flag    = NULL,
		  .val     = 't' },
		{ .name    = "quiet",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 'q' },
		{ .name    = "verbose",
		  .has_arg = no_argument,
		  .flag    = NULL,
		  .val     = 'v' },
		{0},
	};

	/* by default we are not verbose */
	sn_setverbosity(VERBOSITY_NORMAL);

	/* Before we parse options, invalidate the override type so it
	 * doesn't get confused later */
	cfg->override_forgetype = -1;

	/* Start off by pre-populating the config structure */
	readenv(cfg);

	while ((ch = getopt_long(*argc, *argv, "+a:r:cqvt:", options, NULL)) != -1) {
		switch (ch) {
		case 'a': {
			cfg->override_default_account = optarg;
		} break;
		case 'r': {
			cfg->override_remote = optarg;
		} break;
		case 'c': {
			cfg->force_colours = 1;
		} break;
		case 'q': {
			sn_setverbosity(VERBOSITY_QUIET);
		} break;
		case 'v': {
			sn_setverbosity(VERBOSITY_VERBOSE);
		} break;
		case 't': {
			if (strcmp(optarg, "github") == 0) {
				cfg->override_forgetype = GCLI_FORGE_GITHUB;
			} else if (strcmp(optarg, "gitlab") == 0) {
				cfg->override_forgetype = GCLI_FORGE_GITLAB;
			} else if (strcmp(optarg, "gitea") == 0) {
				cfg->override_forgetype = GCLI_FORGE_GITEA;
			} else if (strcmp(optarg, "bugzilla") == 0) {
				cfg->override_forgetype = GCLI_FORGE_BUGZILLA;
			} else {
				fprintf(stderr, "gcli: error: unknown forge type '%s'. "
				        "Have either github, gitlab or gitea.\n", optarg);
				return EXIT_FAILURE;
			}
		} break;
		case 0: break;
		case '?':
		default:
			return EXIT_FAILURE;
		}
	}

	*argc -= optind;
	*argv += optind;

	/* This one is a little odd: We are going to call getopt_long
	 * again. Eventually. But since this is a global variable and the
	 * getopt parser is reusing it, we need to reset it to zero. On
	 * BSDs there is also the optreset variable, but it doesn't exist
	 * on Solaris. I will thus not depend on it as it seems to be
	 * working without it. */
	optind = 0;

	cfg->inited = false;

	return EXIT_SUCCESS;
}

static struct gcli_config_section const *
find_section(struct gcli_config *cfg, char const *name)
{
	struct gcli_config_section *section;

	TAILQ_FOREACH(section, &cfg->sections, next) {
		if (sn_sv_eq_to(section->title, name))
			return section;
	}
	return NULL;
}

struct gcli_config_entries const *
gcli_config_get_section_entries(struct gcli_ctx *ctx, char const *section_name)
{
	struct gcli_config_section const *s;
	struct gcli_config *cfg;

	cfg = ensure_config(ctx);
	s = find_section(cfg, section_name);

	if (s == NULL)
		return NULL;
	else
		return &s->entries;
}

sn_sv
gcli_config_find_by_key(struct gcli_ctx *ctx, char const *section_name,
                        char const *key)
{
	struct gcli_config_entry *entry;
	struct gcli_config *cfg = ensure_config(ctx);

	struct gcli_config_section const *const section =
		find_section(cfg, section_name);

	if (!section) {
		warnx("gcli: no config section with name '%s'", section_name);
		return SV_NULL;
	}

	TAILQ_FOREACH(entry, &section->entries, next) {
		if (sn_sv_eq_to(entry->key, key))
			return entry->value;
	}

	return SV_NULL;
}

static sn_sv
gcli_local_config_find_by_key(struct gcli_ctx *ctx, char const *const key)
{
	struct gcli_dotgcli *lcfg = ctx_dotgcli(ctx);
	struct gcli_config_entry *entry;

	TAILQ_FOREACH(entry, &lcfg->entries, next) {
		if (sn_sv_eq_to(entry->key, key))
			return entry->value;
	}

	return SV_NULL;
}

char *
gcli_config_get_editor(struct gcli_ctx *ctx)
{
	ensure_config(ctx);

	return sn_sv_to_cstr(gcli_config_find_by_key(ctx, "defaults", "editor"));
}

static char const *const
default_account_entry_names[] = {
	[GCLI_FORGE_GITHUB]   = "github-default-account",
	[GCLI_FORGE_GITLAB]   = "gitlab-default-account",
	[GCLI_FORGE_GITEA]    = "gitea-default-account",
	[GCLI_FORGE_BUGZILLA] = "bugzilla-default-account",};

static char *
get_default_account(struct gcli_ctx *ctx, gcli_forge_type ftype)
{
	char const *const defaultname = default_account_entry_names[ftype];
	sn_sv act = gcli_config_find_by_key(ctx, "defaults", defaultname);

	if (!act.length)
		return NULL;

	return sn_sv_to_cstr(act);
}

static char *
gcli_config_get_account(struct gcli_ctx *ctx)
{
	struct gcli_config *cfg = ctx_config(ctx);
	gcli_forge_type ftype = gcli_config_get_forge_type(ctx);
	char *account;

	if (cfg->override_default_account) {
		account = strdup(cfg->override_default_account);
	} else {
		account = get_default_account(ctx, ftype);
	}

	return account;
}

static char const *const default_urls[] = {
	[GCLI_FORGE_GITHUB]   = "https://api.github.com",
	[GCLI_FORGE_GITLAB]   = "https://gitlab.com/api/v4",
	[GCLI_FORGE_GITEA]    = "https://codeberg.org/api/v1",
	[GCLI_FORGE_BUGZILLA] = "https://bugs.freebsd.org/bugzilla",
};

char *
gcli_config_get_apibase(struct gcli_ctx *ctx)
{
	char *acct = gcli_config_get_account(ctx);
	char *url = NULL;

	if (acct) {
		sn_sv url_sv = gcli_config_find_by_key(ctx, acct, "apibase");
		if (url_sv.length)
			url = sn_sv_to_cstr(url_sv);
	}

	if (!url)
		url = strdup(default_urls[gcli_config_get_forge_type(ctx)]);


	free(acct);

	return url;
}

char *
gcli_config_get_account_name(struct gcli_ctx *ctx)
{
	char *account = gcli_config_get_account(ctx);
	sn_sv actname = gcli_config_find_by_key(
		ctx, account, "account");

	free(account);

	return sn_sv_to_cstr(actname);
}

static char *
get_account_token(struct gcli_ctx *ctx)
{
	char *account;
	sn_sv token;

	account = gcli_config_get_account(ctx);
	if (!account)
		return NULL;

	token = gcli_config_find_by_key(ctx, account, "token");

	free(account);

	return sn_sv_to_cstr(token);
}

char *
gcli_config_get_token(struct gcli_ctx *ctx)
{
	ensure_config(ctx);

	return get_account_token(ctx);
}

sn_sv
gcli_config_get_upstream(struct  gcli_ctx *ctx)
{
	init_local_config(ctx);

	return gcli_local_config_find_by_key(ctx, "pr.upstream");
}

bool
gcli_config_pr_inhibit_delete_source_branch(struct gcli_ctx *ctx)
{
	sn_sv val;

	init_local_config(ctx);

	val = gcli_local_config_find_by_key(ctx, "pr.inhibit-delete-source-branch");

	return sn_sv_eq_to(val,	"yes");
}

void
gcli_config_get_upstream_parts(struct gcli_ctx *ctx, sn_sv *const owner,
                               sn_sv *const repo)
{
	ensure_config(ctx);

	sn_sv upstream = gcli_config_get_upstream(ctx);
	*owner = sn_sv_chop_until(&upstream, '/');

	/* Sanity check: did we actually reach the '/'? */
	if (*upstream.data != '/')
		errx(1, "gcli: .gcli has invalid upstream format. expected owner/repo");

	upstream.data   += 1;
	upstream.length -= 1;
	*repo            = upstream;
}

sn_sv
gcli_config_get_base(struct gcli_ctx *ctx)
{
	init_local_config(ctx);

	return gcli_local_config_find_by_key(ctx, "pr.base");
}

sn_sv
gcli_config_get_override_default_account(struct gcli_ctx *ctx)
{
	struct gcli_config *cfg;

	init_local_config(ctx);
	cfg = ctx_config(ctx);

	if (cfg->override_default_account)
		return SV((char *)cfg->override_default_account);
	else
		return SV_NULL;
}

static gcli_forge_type
gcli_config_get_forge_type_internal(struct gcli_ctx *ctx)
{
	struct gcli_config *cfg = ctx_config(ctx);

	/* Hard override */
	if (cfg->override_forgetype >= 0)
		return cfg->override_forgetype;

	ensure_config(ctx);
	init_local_config(ctx);

	sn_sv entry = {0};

	if (cfg->override_default_account) {
	    char const *section = cfg->override_default_account;
		entry = gcli_config_find_by_key(ctx, section, "forge-type");
		if (sn_sv_null(entry))
			errx(1,
			     "gcli: error: given default override account not found or "
			     "missing forge-type");
	} else {
		entry = gcli_local_config_find_by_key(ctx, "forge-type");
	}

	if (!sn_sv_null(entry)) {
		if (sn_sv_eq_to(entry, "github"))
			return GCLI_FORGE_GITHUB;
		else if (sn_sv_eq_to(entry, "gitlab"))
			return GCLI_FORGE_GITLAB;
		else if (sn_sv_eq_to(entry, "gitea"))
			return GCLI_FORGE_GITEA;
		else if (sn_sv_eq_to(entry, "bugzilla"))
			return GCLI_FORGE_BUGZILLA;
		else
			errx(1, "gcli: unknown forge type "SV_FMT, SV_ARGS(entry));
	}

	/* As a last resort, try to infer from the git remote */
	int const type = gcli_gitconfig_get_forgetype(ctx, cfg->override_remote);
	if (type < 0)
		errx(1, "gcli: error: cannot infer forge type. "
		     "use -t <forge-type> to overrride manually.");

	return type;
}

gcli_forge_type
gcli_config_get_forge_type(struct gcli_ctx *ctx)
{
	gcli_forge_type const result = gcli_config_get_forge_type_internal(ctx);

	/* print the type if verbose */
	if (sn_verbose()) {
		static int have_printed_forge_type = 0;
		static char const *const ftype_name[] = {
			[GCLI_FORGE_GITHUB] = "GitHub",
			[GCLI_FORGE_GITLAB] = "Gitlab",
			[GCLI_FORGE_GITEA] = "Gitea",
			[GCLI_FORGE_BUGZILLA] = "Bugzilla",
		};

		if (!have_printed_forge_type) {
			have_printed_forge_type = 1;
			fprintf(stderr, "gcli: info: forge type is %s\n", ftype_name[result]);
		}
	}

	return result;
}

void
gcli_config_get_repo(struct gcli_ctx *ctx, char const **const owner,
                     char const **const repo)
{
	sn_sv upstream = {0};
	struct gcli_config *cfg;

	cfg = ensure_config(ctx);

	if (cfg->override_remote) {
		int const forge = gcli_gitconfig_repo_by_remote(
			cfg->override_remote, owner, repo);

		if (forge >= 0) {
			if ((int)(gcli_config_get_forge_type(ctx)) != forge)
				errx(1, "gcli: error: forge types are inconsistent");
		}

		return;
	}

	if ((upstream = gcli_config_get_upstream(ctx)).length != 0) {
		sn_sv const owner_sv = sn_sv_chop_until(&upstream, '/');
		sn_sv const repo_sv  = sn_sv_from_parts(
			upstream.data + 1,
			upstream.length - 1);

		*owner = sn_sv_to_cstr(owner_sv);
		*repo  = sn_sv_to_cstr(repo_sv);

		return;
	}

	gcli_gitconfig_repo_by_remote(NULL, owner, repo);
}

int
gcli_config_have_colours(struct gcli_ctx *ctx)
{
	static int tested_tty = 0;
	struct gcli_config *cfg;

	cfg = ctx_config(ctx);

	if (cfg->force_colours)
		return 1;

	if (cfg->colours_disabled)
		return 0;

	if (tested_tty)
		return !cfg->colours_disabled;

	if (isatty(STDOUT_FILENO))
		cfg->colours_disabled = false;
	else
		cfg->colours_disabled = true;

	tested_tty = 1;

	return !cfg->colours_disabled;
}
