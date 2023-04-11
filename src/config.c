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

#include <gcli/config.h>
#include <gcli/github/config.h>
#include <gcli/gitlab/config.h>
#include <gcli/gitea/config.h>
#include <gcli/gitconfig.h>
#include <gcli/forges.h>

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif
#include <stdlib.h>
#include <unistd.h>

#define CONFIG_MAX_ENTRIES 16
struct gcli_config_section {
	struct gcli_config_entry {
		sn_sv key;
		sn_sv value;
	} entries[CONFIG_MAX_ENTRIES];
	size_t entries_size;

	sn_sv title;
};

#define CONFIG_MAX_SECTIONS 16
static struct gcli_config {
	struct gcli_config_section sections[CONFIG_MAX_SECTIONS];
	size_t                     sections_size;

	char const *override_default_account;
	char const *override_remote;
	int         override_forgetype;
	int         colours_disabled;     /* NO_COLOR set or output is not a TTY */
	int         force_colours;        /* -c option was given */

	sn_sv  buffer;
	void  *mmap_pointer;
	bool   inited;
} config;

static struct gcli_dotgcli {
	struct gcli_config_entry entries[128];
	size_t entries_size;

	sn_sv  buffer;
	void  *mmap_pointer;
	bool   has_been_searched_for;
	bool   has_been_found;
} local_config;

static bool
should_init_dotgcli(void)
{
	return !local_config.has_been_searched_for ||
		(local_config.has_been_searched_for && !local_config.has_been_found);
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
		err(1, "getcwd");

	/* Here we are trying to traverse upwards through the directory
	 * tree, searching for a directory called .git.
	 * Starting point is ".".*/
	do {
		curr_dir = opendir(curr_dir_path);
		if (!curr_dir)
			err(1, "opendir");

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
				err(1, "realpath at %s", tmp);

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
init_local_config(void)
{
	if (!should_init_dotgcli())
		return;

	char const *path = find_dotgcli();
	if (!path) {
		local_config.has_been_searched_for = true;
		local_config.has_been_found        = false;
		return;
	}

	local_config.has_been_searched_for = true;
	local_config.has_been_found        = true;

	int len = sn_mmap_file(path, &local_config.mmap_pointer);
	if (len < 0)
		err(1, "Unable to open config file");

	local_config.buffer = sn_sv_from_parts(local_config.mmap_pointer, len);
	local_config.buffer = sn_sv_trim_front(local_config.buffer);

	int curr_line = 1;
	while (local_config.buffer.length > 0) {
		sn_sv line = sn_sv_chop_until(&local_config.buffer, '\n');

		line = sn_sv_trim(line);

		if (line.length == 0)
			errx(1, "%s:%d: Unexpected end of line",
			     path, curr_line);

		// Comments
		if (line.data[0] == '#') {
			local_config.buffer = sn_sv_trim_front(local_config.buffer);
			curr_line++;
			continue;
		}

		sn_sv key  = sn_sv_chop_until(&line, '=');

		key = sn_sv_trim(key);

		if (key.length == 0)
			errx(1, "%s:%d: empty key", path, curr_line);

		line.data   += 1;
		line.length -= 1;

		sn_sv value = sn_sv_trim(line);

		local_config.entries[local_config.entries_size].key   = key;
		local_config.entries[local_config.entries_size].value = value;
		local_config.entries_size++;

		local_config.buffer = sn_sv_trim_front(local_config.buffer);
		curr_line++;
	}

	free((void *)path);
}

struct config_parser {
	sn_sv       buffer;
	int         line;
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
parse_keyvaluepair(struct config_parser *input, struct gcli_config_entry *out)
{
	sn_sv key = sn_sv_chop_until(&input->buffer, '=');

	if (key.length == 0)
		errx(1, "%s:%d: empty key", input->filename, input->line);

	input->buffer.data   += 1;
	input->buffer.length -= 1;

	sn_sv value = sn_sv_chop_until(&input->buffer, '\n');

	out->key   = sn_sv_trim(key);
	out->value = sn_sv_trim(value);
}

static sn_sv
parse_section_title(struct config_parser *input)
{
	size_t len = 0;
	if (input->buffer.length == 0)
		errx(1, "%s:%d: unexpected end of input in section title",
		     input->filename, input->line);


	while (!isspace(input->buffer.data[len]) && input->buffer.data[len] != '{')
		len++;

	sn_sv title = sn_sv_from_parts(input->buffer.data, len);
	input->buffer.data   += len;
	input->buffer.length -= len;

	skip_ws_and_comments(input);

	if (input->buffer.length == 0)
		errx(1, "%s:%d: unexpected end of input", input->filename, input->line);

	if (input->buffer.data[0] != '{')
		errx(1, "%s:%d: expected '{'", input->filename, input->line);

	input->buffer.length -= 1;
	input->buffer.data   += 1;

	skip_ws_and_comments(input);
	return title;
}

static void
parse_config_section(struct config_parser *input)
{
	struct gcli_config_section *section = NULL;

	if (config.sections_size == CONFIG_MAX_SECTIONS)
		errx(1, "error: too many config sections");

	section = &config.sections[config.sections_size++];

	section->title = parse_section_title(input);

	while (input->buffer.length > 0 && input->buffer.data[0] != '}') {
		skip_ws_and_comments(input);

		if (section->entries_size == CONFIG_MAX_ENTRIES)
			errx(1, "error: too many config entries in section "SV_FMT,
			     SV_ARGS(section->title));

		parse_keyvaluepair(input, &section->entries[section->entries_size++]);
		skip_ws_and_comments(input);
	}

	if (input->buffer.length == 0)
		errx(1, "%s:%d: missing '}' before end of file",
		     input->filename, input->line);

	input->buffer.length -= 1;
	input->buffer.data   += 1;
}

static void
parse_config_file(struct config_parser *input)
{
	skip_ws_and_comments(input);

	while (input->buffer.length > 0) {
		parse_config_section(input);
		skip_ws_and_comments(input);
	}
}

/**
 * Try to load up the local config file if it exists. If we succeed,
 * return 0. Otherwise return -1.
 */
static int
ensure_config(void)
{
	char                 *file_path = NULL;
	struct config_parser  parser    = {0};

	if (config.inited)
		return 0;

	config.inited = true;

	file_path = getenv("XDG_CONFIG_PATH");
	if (!file_path) {
		file_path = getenv("HOME");
		if (!file_path) {
			warnx("Neither XDG_CONFIG_PATH nor HOME set in env");
			return -1;
		}

		/*
		 * Code duplication to avoid leaking pointers */
		file_path = sn_asprintf("%s/.config/gcli/config", file_path);
	} else {
		file_path = sn_asprintf("%s/gcli/config", file_path);
	}

	if (access(file_path, R_OK) < 0) {
		warn("Cannot access config file at %s", file_path);
		return -1;
	}

	int len = sn_mmap_file(file_path, &config.mmap_pointer);
	if (len < 0)
		err(1, "Unable to open config file");

	config.buffer = sn_sv_from_parts(config.mmap_pointer, len);
	config.buffer = sn_sv_trim_front(config.buffer);

	parser.buffer   = config.buffer;
	parser.line     = 1;
	parser.filename = file_path;

	parse_config_file(&parser);

	free((void *)file_path);

	return 0;
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
readenv(void)
{
	char *tmp;

	/* A default override account. Can be overridden again by
	 * specifying -a <account-name> */
	if ((tmp = getenv("GCLI_ACCOUNT")))
		config.override_default_account = tmp;

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
		config.colours_disabled = checkyes(tmp);
}

int
gcli_config_init(int *argc, char ***argv)
{
	/* These are the very first options passed to the gcli command
	 * itself. It is the first ever getopt call we do to parse any
	 * arguments. Only global options that do not alter subcommand
	 * specific behaviour should be accepted here. */

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
		  .flag    = &config.colours_disabled,
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
	config.override_forgetype = -1;

	/* Start off by pre-populating the config structure */
	readenv();

	while ((ch = getopt_long(*argc, *argv, "+a:r:cqvt:", options, NULL)) != -1) {
		switch (ch) {
		case 'a': {
			config.override_default_account = optarg;
		} break;
		case 'r': {
			config.override_remote = optarg;
		} break;
		case 'c': {
			config.force_colours = 1;
		} break;
		case 'q': {
			sn_setverbosity(VERBOSITY_QUIET);
		} break;
		case 'v': {
			sn_setverbosity(VERBOSITY_VERBOSE);
		} break;
		case 't': {
			if (strcmp(optarg, "github") == 0) {
				config.override_forgetype = GCLI_FORGE_GITHUB;
			} else if (strcmp(optarg, "gitlab") == 0) {
				config.override_forgetype = GCLI_FORGE_GITLAB;
			} else if (strcmp(optarg, "gitea") == 0) {
				config.override_forgetype = GCLI_FORGE_GITEA;
			} else {
				fprintf(stderr, "error: unknown forge type '%s'. "
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

	config.inited = false;

	return EXIT_SUCCESS;
}

static struct gcli_config_section const *
find_section(sn_sv name)
{
	for (size_t i = 0; i < config.sections_size; ++i) {
		if (sn_sv_eq(config.sections[i].title, name))
			return &config.sections[i];
	}
	return NULL;
}

sn_sv
gcli_config_find_by_key(sn_sv const section_name, char const *key)
{
	ensure_config();

	struct gcli_config_section const *const section =
		find_section(section_name);

	if (!section) {
		warnx("no config section with name '"SV_FMT"'", SV_ARGS(section_name));
		return SV_NULL;
	}

	for (size_t i = 0; i < section->entries_size; ++i)
		if (sn_sv_eq_to(section->entries[i].key, key))
			return section->entries[i].value;

	return SV_NULL;
}

static sn_sv
gcli_local_config_find_by_key(char const *const key)
{
	for (size_t i = 0; i < local_config.entries_size; ++i)
		if (sn_sv_eq_to(local_config.entries[i].key, key))
			return local_config.entries[i].value;
	return SV_NULL;
}

char *
gcli_config_get_editor(void)
{
	ensure_config();

	return sn_sv_to_cstr(gcli_config_find_by_key(SV("defaults"), "editor"));
}

char *
gcli_config_get_authheader(void)
{
	ensure_config();

	return gcli_forge()->get_authheader();
}

sn_sv
gcli_config_get_account(void)
{
	ensure_config();

	return gcli_forge()->get_account();
}

sn_sv
gcli_config_get_upstream(void)
{
	init_local_config();

	return gcli_local_config_find_by_key("pr.upstream");
}

bool
gcli_config_pr_inhibit_delete_source_branch(void)
{
	sn_sv val;

	init_local_config();

	val = gcli_local_config_find_by_key("pr.inhibit-delete-source-branch");
	return sn_sv_eq_to(val,	"yes");
}

void
gcli_config_get_upstream_parts(sn_sv *const owner, sn_sv *const repo)
{
	ensure_config();

	sn_sv upstream   = gcli_config_get_upstream();
	*owner           = sn_sv_chop_until(&upstream, '/');

	/* Sanity check: did we actually reach the '/'? */
	if (*upstream.data != '/')
		errx(1, ".gcli has invalid upstream format. expected owner/repo");

	upstream.data   += 1;
	upstream.length -= 1;
	*repo            = upstream;
}

sn_sv
gcli_config_get_base(void)
{
	init_local_config();

	return gcli_local_config_find_by_key("pr.base");
}

sn_sv
gcli_config_get_override_default_account(void)
{
	init_local_config();

	if (config.override_default_account)
		return SV((char *)config.override_default_account);
	else
		return SV_NULL;
}

static gcli_forge_type
gcli_config_get_forge_type_internal(void)
{
	/* Hard override */
	if (config.override_forgetype >= 0)
		return config.override_forgetype;

	ensure_config();
	init_local_config();

	sn_sv entry = {0};

	if (config.override_default_account) {
		sn_sv const section = SV((char *)config.override_default_account);
		entry = gcli_config_find_by_key(section, "forge-type");
		if (sn_sv_null(entry))
			errx(1,
			     "error: given default override account not found or "
			     "missing forge-type");
	} else {
		entry = gcli_local_config_find_by_key("forge-type");
	}

	if (!sn_sv_null(entry)) {
		if (sn_sv_eq_to(entry, "github"))
			return GCLI_FORGE_GITHUB;
		else if (sn_sv_eq_to(entry, "gitlab"))
			return GCLI_FORGE_GITLAB;
		else if (sn_sv_eq_to(entry, "gitea"))
			return GCLI_FORGE_GITEA;
		else
			errx(1, "Unknown forge type "SV_FMT, SV_ARGS(entry));
	}

	/* As a last resort, try to infer from the git remote */
	int const type = gcli_gitconfig_get_forgetype(config.override_remote);
	if (type < 0)
		errx(1, "error: cannot infer forge type. "
		     "use -t <forge-type> to overrride manually.");

	return type;
}

gcli_forge_type
gcli_config_get_forge_type(void)
{
	gcli_forge_type const result = gcli_config_get_forge_type_internal();

	/* print the type if verbose */
	if (sn_verbose()) {
		static int have_printed_forge_type = 0;
		static char const *const ftype_name[] = {
			[GCLI_FORGE_GITHUB] = "GitHub",
			[GCLI_FORGE_GITLAB] = "Gitlab",
			[GCLI_FORGE_GITEA]  = "Gitea",
			};

		if (!have_printed_forge_type) {
			have_printed_forge_type = 1;
			fprintf(stderr, "info: forge type is %s\n", ftype_name[result]);
		}
	}

	return result;
}

void
gcli_config_get_repo(char const **const owner, char const **const repo)
{
	sn_sv upstream = {0};

	ensure_config();

	if (config.override_remote) {
		int const forge = gcli_gitconfig_repo_by_remote(
			config.override_remote, owner, repo);

		if (forge >= 0) {
			if ((int)(gcli_config_get_forge_type()) != forge)
				errx(1, "error: forge types are inconsistent");
		}

		return;
	}

	if ((upstream = gcli_config_get_upstream()).length != 0) {
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
gcli_config_have_colours(void)
{
	static int tested_tty = 0;

	if (config.force_colours)
		return 1;

	if (config.colours_disabled)
		return 0;

	if (tested_tty)
		return !config.colours_disabled;

	if (isatty(STDOUT_FILENO))
		config.colours_disabled = false;
	else
		config.colours_disabled = true;

	tested_tty = 1;

	return !config.colours_disabled;
}

char const *
gcli_get_apibase(void)
{
	switch (gcli_config_get_forge_type()) {
	case GCLI_FORGE_GITHUB:
		return github_get_apibase();
	case GCLI_FORGE_GITEA:
		return gitea_get_apibase();
	case GCLI_FORGE_GITLAB:
		return gitlab_get_apibase();
	default:
		assert(0 && "Not reached");
	}
}
