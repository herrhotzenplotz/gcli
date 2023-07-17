# GCLI Tutorial

This document is aimed at those who are new to gcli and want get
started using it.

## Installation

### Through package manager

If you're on FreeBSD you can just install gcli by running the
following command:

    # pkg install gcli

### Compile the source code

Other operating systems currently require manual compilation and
installation.

For this purpose go to
[https://herrhotzenplotz.de/gcli/releases](https://herrhotzenplotz.de/gcli/releases)
and choose the latest release. Then download one of the tarballs.

For version 1.1.0 this would be:

https://herrhotzenplotz.de/gcli/releases/gcli-1.1.0/gcli-1.1.0.tar.xz

Now that you have a link, you can download it, extract it, compile the
code and install it:

    $ mkdir ~/build
    $ cd ~/build
    $ curl -4LO https://herrhotzenplotz.de/gcli/releases/gcli-1.1.0/gcli-1.1.0.tar.xz
      % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                     Dload  Upload   Total   Spent    Left  Speed
    100  342k  100  342k    0     0  2739k      0 --:--:-- --:--:-- --:--:-- 2736k
    $ ls
    gcli-1.1.0.tar.xz
    $

Install the dependencies for building gcli:

e.g. on Debian systems:

    # apt install libcurl4-openssl-dev pkgconf build-essential

Extract the tarball:

    $ tar xf gcli-1.1.0.tar.xz
    $ cd gcli-1.1.0

Configure, build and install gcli:

    $ ./configure --prefix=${HOME}/.local
    ...
    $ make
    ...
    $ make install

Now put the gcli executable into your PATH variable to allow the shell
to easily find it:

    $ PATH=${PATH}:${HOME}/.local/bin
    $ export PATH

You can put the above commands into your shell initialisation file, e.g.:

    $ echo 'PATH=${PATH}:${HOME}/.local/bin; export PATH' >> ~/.shrc

Check that the shell finds gcli:

    $ which gcli
    /usr/home/nico/.local/bin/gcli
    $
    $ gcli version
    gcli 1.1.0 (amd64-unknown-freebsd13.2)
    Using libcurl/8.1.2 OpenSSL/1.1.1t zlib/1.2.13 libpsl/0.21.2 (+libidn2/2.3.4) libssh2/1.11.0 nghttp2/1.53.0
    Using vendored pdjson library
    Report bugs at https://gitlab.com/herrhotzenplotz/gcli/.
    Copyright 2021, 2022, 2023 Nico Sonack <nsonack@herrhotzenplotz.de> and contributors.
    $

Furthermore I recommend setting the `MANPATH` variable so that you can
easily read the installed manual pages:

    $ MANPATH=:${HOME}/.local/share/man; export MANPATH

You can also put this into your shell initialisation file.

## First steps

### Issues

Let's start off by listing some issues - here for the curl project
which is hosted on GitHub under curl/curl. To list issues for it one
would run:

    $ gcli -t github issues -o curl -r curl

You will see the list of the 30 most recent open issue tickets. The
command above does the following:

  - invoke gcli
  - as a global option we switch it into Github-Mode
  - invoke the issues subcommand
  - operate on the repository owner curl (`-o curl`)
  - operate on the repository curl (`-r curl`)

Note that the `-t github` option goes before the issues subcommand
because it is a global option for gcli that affects how all the
following things like subcommands operate.

However, now I also want to see closed issues:

    $ gcli -t github issues -o curl -r curl -a

The `-a` option will disregard the status of the issue.

Oh and the screen is a bit cluttered by all these tickets - let's only
fetch the first 10 issues:

    $ gcli -t github issues -o curl -r curl -n10

#### Details about issues

As of now we only produced lists of issues. However, we may also want
to look at the details of an issue such as:

  - the original post
  - labels
  - comments
  - assignees of the issue (that is someone who is working on the bug)

Let's get a good summary of issue `#11268` in the curl project:

    $ gcli -t github issues -o curl -r curl -i 11268 all

As you can see most of the options are the same, however now we tell
gcli with the `-i 11268` option that we want to work with a single
issue. Then we tell gcli what actions to perform on the issue. Another
important action is `comments`. Guess what it does:

    $ gcli -t github issues -o curl -r curl -i 11268 comments

I know a person that likes to post long verbose traces. Let's search
for an issue authored by them on the OpenSSL GitHub page:

    $ gcli -t github issues -o openssl -r openssl -A blastwave -a
    NUMBER  STATE   TITLE
     20379  open    test "80-test_ssl_new.t" fails on Solaris 10 SPARCv9
     10547  open    Strict C90 CFLAGS results in sha.h:91 ISO C90 does not support long long
      8048  closed  OPENSSL_strnlen SIGSEGV in o_str.c line 76
    $

The `-A` option lets you filter for specific authors.

Let's look at the issue state of `#10547`:

    $ gcli -t github issues -o openssl -r openssl -i 10547 status
         NAME : 10547
        TITLE : Strict C90 CFLAGS results in sha.h:91 ISO C90 does not support long long
      CREATED : 2019-12-01T04:35:23Z
       AUTHOR : blastwave
        STATE : open
     COMMENTS : 9
       LOCKED : no
       LABELS : triaged: bug
    ASSIGNEES : none
    $

That's nine comments - let's read the original post and the comments
in our favourite pager `less`:

    $ gcli -t github issues -o openssl -r openssl -i 10547 op comments | less

As you can see gcli will accept multiple actions for an issue and
executes them sequentially.

#### Further reading

You can list all available options for the issues subcommand by doing:

    $ gcli issues --help

With your current knowledge you can also explore the `gcli pulls` subcommand.

Furthermore I recommend reading into the manual page `gcli-issues(1)`
and `gcli-pulls(1)`:

    $ man gcli-issues
    $ man gcli-pulls

### Creating issues

Creating issues on Github requires an account which we need to
generate an authentication token for gcli.

Log into your GitHub account and click on your account icon in the top
right corner. Then choose the `Settings` option. Scroll down and
choose `Developer settings` on the bottom of the left column. Under
`Personal access tokens` choose `Tokens (classic)`.

Click on `Generate new token (classic)`.

Set a useful name such as `gcli` in the Note field, set the expiration
to `No expiration` and allow the following `Scopes`:

  - `repo`
  - `workflow`
  - `admin:public_key`
  - `gist`

Then create the token. It'll be printed in green. Do not share it!

Now we need to tell gcli about this new token. To do this, create a
configuration file for gcli:

    $ mkdir -p ${HOME}/.config/gcli
    $ vi ${HOME}/.config/gcli/config

Obviously, you can choose any other editor of your choice. Put the
following into this file:


    defaults {
        editor=vi
        github-default-account=my-github-account
    }

    my-github-account {
        token=<token-goes-here>
        account=<account-name>
        forge-type=github
    }

Replace the `<token-goes-here>` with the previously generated token
and the `<account>` with your account name.

If you now run

    $ gcli -t github repos

you should get a list of your repos. If not, check again that you did
all the steps above correctly.

**To be continued**
