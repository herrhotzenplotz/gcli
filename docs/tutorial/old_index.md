# GCLI Tutorial

This document is aimed at those who are new to gcli and want get
started using it.

## Table of contents

1. [Installing GCLI](./02-Installation.html)
1. [First steps](./03-First-Steps.html)

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

### First issue

For this case I have a playground repository that you may as well use
for testing with gcli. It is available at
`herrhotzenplotz/ghcli-playground`.

To see a list of issues, we can run:

    $ gcli -t github issues -o herrhotzenplotz -r ghcli-playground -a
    NUMBER  NOTES  STATE   TITLE
        13      0  open    yet another issue
        12      0  closed  wat
        11      0  closed  blaaaaaaaaaaaaaaaaaaaah
        10      0  closed  "this is the quoted" issue title? anyone?"
         9      0  closed  test
         8      0  closed  foobar
         7      0  closed  foobar
         5      0  closed  test2
         4      0  closed  test
    $

#### Invoke gcli
Let's create a bug report where we complain about things not working:

    $ gcli -t github issues create -o herrhotzenplotz -r ghcli-playground \
        "Bug: Doesn't work on my machine"

The message "Bug: doesn't work on my machine" is the title of the
issue.

#### Original Post

You will see the default editor come up and instruct you to type in a
message. This message is the "original post" or the body of the issue
ticket that you're about to submit. You can use Markdown Syntax:

     I tried building this code on my machine but unfortunately it errors
     out with the following message:

     ```console
     $ make love
     make: don't know how to make love. Stop

     make: stopped in /tmp/wat
     $
     ```

     What am I doing wrong?

     ! ISSUE TITLE : Bug: Doesn't work on my machine
     ! Enter issue description above.
     ! All lines starting with '!' will be discarded.

#### Submit the issue

After you save and exit the editor gcli gives you a chance to check
back and finally submit the issue. Type 'y' and hit enter.

You can check back if the issue was created and also view details
about it as you learned earlier.
