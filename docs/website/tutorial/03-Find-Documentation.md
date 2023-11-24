# How to find documentation

When using gcli one may not always remember all the options and flags
for every subcommand. gcli has lots of integrated help to guide you
through its commands.

## Subcommand help

You can list all available options for the issues subcommand by doing:

    $ gcli issues --help

With your current knowledge you can also explore the `gcli pulls` subcommand.

## General usage

Run the following command:

    $ gcli --help
    usage: gcli [options] subcommand

    OPTIONS:
      -a account     Use the configured account instead of inferring it
      -r remote      Infer account from the given git remote
      -t type        Force the account type:
                        - github (default: github.com)
                        - gitlab (default: gitlab.com)
                        - gitea (default: codeberg.org)
      -c             Force colour and text formatting.
      -q             Be quiet. (Not implemented yet)

      -v             Be verbose.

    SUBCOMMANDS:
      ci             Github CI status info
      comment        Comment under issues and PRs
      config         Configure forges
      forks          Create, delete and list repository forks
      gists          Create, fetch and list Github Gists
      issues         Manage issues
      labels         Manage issue and PR labels
      milestones     Milestone handling
      pipelines      Gitlab CI management
      pulls          Create, view and manage PRs
      releases       Manage releases of repositories
      repos          Remote Repository management
      snippets       Fetch and list Gitlab snippets
      status         General user status and notifications
      api            Fetch plain JSON info from an API (for debugging purposes)
      version        Print version

    gcli 1.2.0 (amd64-unknown-freebsd13.2)
    Using libcurl/8.1.2 OpenSSL/1.1.1t zlib/1.2.13 libpsl/0.21.2 (+libidn2/2.3.4) libssh2/1.11.0 nghttp2/1.53.0
    Using vendored pdjson library
    Report bugs at https://gitlab.com/herrhotzenplotz/gcli/.
    Copyright 2021, 2022, 2023 Nico Sonack <nsonack@herrhotzenplotz.de> and contributors.


This gives you an overview over all the available subcommands. Each
subcommand in turn allows you to get its usage by supplying the
`--help` option to it.

## Manual pages

Furthermore I recommend reading into the manual page `gcli-issues(1)`
and `gcli-pulls(1)`:

    $ man gcli-issues
    $ man gcli-pulls
