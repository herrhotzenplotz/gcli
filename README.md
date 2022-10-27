# GCLI

Simple and portable CLI tool for interacting with GitHub, GitLab and
Gitea from the command line.

![](docs/screenshot-02.png)

## Why?

The official GitHub CLI tool only supports GitHub. I wanted a simple
unified tool for various git forges such as GitHub and GitLab because
every forge does things differently yet all build on Git and
purposefully break with its philosophy.

Also, the official tool from Github is written in Go, which does
manual [DNS
resolution](https://github.com/golang/go/blob/master/src/net/dnsclient_unix.go#L49)
which is a massive security vulnerability for people using Tor as it
leaks your IP to the DNS server. This program builds upon libcurl,
which obeys the operating system's DNS resolution mechanisms and thus
also works with Tor.

## Building

### Download

Recent tarballs can be downloaded here:

[https://herrhotzenplotz.de/gcli/releases/](https://herrhotzenplotz.de/gcli/releases/)

### Dependencies

Required dependencies:
- libcurl
- pkg-config
- yacc (System V yacc, Berkeley Yacc or Bison should suffice)
- lex (flex is preferred)
- C99 Compiler and linker
- make

If you are building from Git you will also need:
- autoconf
- automake

### Compile
In order to perform a build, do:
```console
$ ./configure
$ make
# make DESTDIR=/ install
```

You may leave out `DESTDIR`. The above is the default value.

If you are building from Git you need to generate the configure script
first:
```console
$ autoreconf -i
```

In case any of this does not work, please either report a bug, or
submit a patch in case you managed to fix it.

Tested Operating Systems so far:
- FreeBSD 13.0-RELEASE amd64 and arm64
- Solaris 10 and 11, sparc64
- Devuan GNU/Linux Chimaera x86_64
- Debian GNU/Linux 5.18.11 ppc64
- Fedora 34 x86_64
- Haiku x86_64
- Minix 3.4.0 (GENERIC) i386
- OpenBSD 7.0 GENERIC amd64
- Alpine Linux 3.16 x86_64

## Support

You can ask your local frenchman aka. neutaaaaan for emotional support
when using this piece of software. Otherwise you can read the man page
at »man gcli«.

## Bugs and contributions

Please report bugs to nsonack@herrhotzenplotz.de or on
[GitLab](https://gitlab.com/herrhotzenplotz/gcli). You can also submit
patches this way using git-send-email.

## License

BSD-2 CLAUSE (aka. FreeBSD License). Please see the LICENSE file
attached.

## Credits

This program makes heavy use of both [libcurl](https://curl.haxx.se/)
and [pdjson](https://github.com/skeeto/pdjson).

herrhotzenplotz aka. Nico Sonack
October 2021
