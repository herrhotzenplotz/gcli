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
#include <gcli/gcli.h>
#include <gcli/port/util.h>

#include <gcli/attachments.h>

#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

static void
usage(void)
{
	fprintf(stderr, "usage: gcli [options] attachments create -s <summary> -i <bug-id>\n");
	fprintf(stderr, "            -f <file> [-c <comment>] [-C content-type] [-P] [-p]\n");
	fprintf(stderr, "       gcli [options] attachments -i <id> actions...\n");
	fprintf(stderr, "OPTIONS:\n");
	fprintf(stderr, "  -c comment       Comment of the attachment to create\n");
	fprintf(stderr, "  -C content-type  Content type of the attachment\n");
	fprintf(stderr, "  -f file          Upload the given file as attachment\n");
	fprintf(stderr, "  -i bug-id        Create the attachment for the given bug ID\n");
	fprintf(stderr, "  -i id            Execute the given actions for the specified attachment id\n");
	fprintf(stderr, "  -p               The attachment is a patch\n");
	fprintf(stderr, "  -P               The attachment is private\n");
	fprintf(stderr, "  -s summary       Summary of the attachment to create\n");
	fprintf(stderr, "ACTIONS:\n");
	fprintf(stderr, "  get [-o path]  Fetch and dump the contents of the "
	                  "attachments to the given path or stdout\n");
	fprintf(stderr, "\n");
	version();
	copyright();
}

static int
action_attachment_get(struct gcli_path const *path,
                      void *item,
                      int *argc, char ***argv)
{
	int ch, rc = 0;
	bool oflag_seen = false;
	FILE *outfile = NULL;

	(void) item;

	struct option options[] = {
		{ .name = "output", .has_arg = required_argument, .flag = NULL, .val = 'o' },
		{0},
	};

	while ((ch = getopt_long(*argc, *argv, "+o:", options, NULL)) != -1) {
		switch (ch) {
		case 'o': {
			outfile = fopen(optarg, "w");
			if (!outfile) {
				fprintf(stderr, "gcli: failed to open '%s': %s\n",
				        optarg, strerror(errno));
				return GCLI_EX_DATAERR;
			}
			oflag_seen = true;
		} break;
		default: {
			return GCLI_EX_USAGE;
		} break;
		}
	}

	*argc -= optind - 1;
	*argv += optind - 1;
	optind = 0; /* reset */

	/* -o wasn't specified */
	if (outfile == NULL)
		outfile = stdout;

	rc = gcli_attachment_get_content(g_clictx, path, outfile);
	if (rc < 0) {
		fprintf(stderr, "gcli: failed to get attachment: %s\n",
		        gcli_get_error(g_clictx));
		return GCLI_EX_DATAERR;
	}

	if (oflag_seen)
		fclose(outfile);

	outfile = NULL;

	return GCLI_EX_OK;
}

static struct gcli_cmd_actions const actions =
{
	.fetch_item = NULL,
	.free_item = NULL,
	.item_size = sizeof(struct gcli_attachment),

	.defs = {
		{ .name = "get",
		  .help = "Print or download attachment",
		  .needs_item = false,
		  .handler = action_attachment_get, },
	},
};

static int
subcommand_attachments_create(int argc, char *argv[])
{
	int ch = 0, rc = 0;
	struct gcli_attachment_create_opts flags = {0};

	struct option const options[] = {
		{ .name = "comment",      .has_arg = required_argument, .val = 'c' },
		{ .name = "content-type", .has_arg = required_argument, .val = 'C' },
		{ .name = "file",         .has_arg = required_argument, .val = 'f' },
		{ .name = "filename",     .has_arg = required_argument, .val = 'F' },
		{ .name = "patch",        .has_arg = no_argument,       .val = 'p' },
		{ .name = "private",      .has_arg = no_argument,       .val = 'P' },
		{ .name = "summary",      .has_arg = required_argument, .val = 's' },
		{ .name = "id",           .has_arg = required_argument, .val = 'i' },
		{0},
	};

	while ((ch = getopt_long(argc, argv, "+s:c:C:pPf:F:i:", options, NULL)) != -1) {
		switch (ch) {
		case 's':
			flags.summary = optarg;
			break;
		case 'c':
			flags.comment = optarg;
			break;
		case 'C':
			flags.content_type = optarg;
			break;
		case 'p':
			flags.is_patch = true;
			break;
		case 'P':
			flags.is_private = true;
			break;
		case 'F':
			flags.file_name = optarg;
			break;
		case 'f':
			if (flags.data)
				errx(1, "gcli: file may only be specified once");

			int rc = gcli_read_file(optarg, (char **)&flags.data);
			if (rc < 0)
				err(1, "gcli: connot read file");

			flags.data_size = rc;

			/* record file name if needed */
			if (flags.file_name == NULL) {
				flags.file_name = strrchr(optarg, '/');
				if (flags.file_name)
					flags.file_name += 1;
				else
					flags.file_name = optarg;
			}

			break;
		case 'i':
			if (gcli_cmd_parse_id(optarg, &flags.bug_id) < 0)
				errx(1, "gcli: cannot parse bug id");
			break;
		default:
			usage();
			return 1;
		}
	}

	if (flags.data == NULL) {
		fprintf(stderr, "gcli: missing file content, use -f\n");
		usage();
		return 1;
	}

	rc = gcli_attachment_create(g_clictx, &flags);
	if (rc < 0)
		errx(1, "gcli: cannot create attachment: %s",
		     gcli_get_error(g_clictx));

	return 0;
}

int
subcommand_attachments(int argc, char *argv[])
{
	bool iflag_seen = false;
	int ch, rc = 0;
	struct gcli_path path = { .kind = GCLI_PATH_ID };

	struct option options[] = {
		{ .name = "id", .has_arg = required_argument, .flag = NULL, .val = 'i' },
		{0},
	};

	/* create an attachment? */
	if (argc > 1 && strcmp(argv[1], "create") == 0) {
		shift(&argc, &argv);
		return subcommand_attachments_create(argc, argv);
	}

	while ((ch = getopt_long(argc, argv, "+i:", options, NULL)) != -1) {
		switch (ch) {
		case 'i': {
			iflag_seen = true;

			if (gcli_cmd_parse_id(optarg, &path.as_id) < 0)
				err(1, "gcli: error: cannot parse attachment id");
		} break;
		default:
			usage();
			return EXIT_FAILURE;
		}
	}

	argc -= optind;
	argv += optind;

	optind = 0;  /* reset */

	if (!iflag_seen) {
		fprintf(stderr, "gcli: missing -i flag\n");
		usage();
		return EXIT_FAILURE;
	}

	if (argc == 0) {
		fprintf(stderr, "gcli: missing actions\n");
		usage();
		return EXIT_FAILURE;
	}

	rc = gcli_cmd_actions_handle(&actions, &path, &argc, &argv);
	if (rc == GCLI_EX_USAGE)
		usage();

	return !!rc;
}
