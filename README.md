# GCLI

Portable CLI tool for interacting with Git(Hub|Lab|Tea), Forgejo and Bugzilla from the command line.

![](docs/screenshot.png)

## Why?

The official GitHub CLI tool only supports GitHub.
I wanted a simple unified tool for various git forges such as GitHub and GitLab because every forge does things differently yet all build on Git and purposefully break with its philosophy.

## Building

### Download

Recent tarballs can be downloaded here:

[https://herrhotzenplotz.de/gcli/releases/](https://herrhotzenplotz.de/gcli/releases/)

There are official packages available:

<a href="https://repology.org/project/gcli/versions">
<img src="https://repology.org/badge/vertical-allrepos/gcli.svg" alt="Packaging status" align="right">
</a>

### Dependencies

Required dependencies:

- libcurl
- yacc (System V yacc, Berkeley Yacc or Bison should suffice)
- lex (flex is preferred)
- C11 Compiler and linker
- make
- pkgconf or pkg-config

Optional dependencies:

- liblowdown
- libedit
- libreadline

The test suite requires:

- [Kyua](https://github.com/jmmv/kyua)
- [ATF](https://github.com/jmmv/atf)

### Compile

In order to perform a build, do:

```console
$ ./configure [--prefix=/usr/local]
$ make
# make [DESTDIR=/] install
```

You may leave out `DESTDIR` and `--prefix=`.
The above is the default value.
The final installation destination is `$DESTDIR/$PREFIX/...`.

If you are unsure, consult the builtin configure help by running `./configure --help`.

In case any of the above does not work, please either report a bug, or submit a patch in case you managed to fix it.

### Testing

To run the test suite first make sure you have all the necessary dependencies installed (see above).
Then you can run `make check` in the build directory to run the test suite.

For more details also see [HACKING.md](HACKING.md).

### (Previously) known to work platforms

Incomplete list of tested operating systems:

- FreeBSD 13.0-RELEASE amd64 and arm64
- Solaris 10 and 11, sparc64
- SunOS 5.11 i86pc (OmniOS)
- Devuan GNU/Linux Chimaera x86_64
- Debian GNU/Linux ppc64, ppc64le
- Gentoo Linux sparc64, ia64
- Fedora 34 x86_64
- Haiku x86_64
- Minix 3.4.0 (GENERIC) i386
- OpenBSD 7.0 GENERIC amd64
- Alpine Linux 3.16 x86_64
- Darwin 22.2.0 arm64
- Windows 10 (MSYS2 mingw32-w64)
- NetBSD 9.3 amd64, sparc64 and VAX

Tested Compilers so far:

- LLVM Clang (various versions)
- GCC (various versions)
- Oracle DeveloperStudio 12.6
- IBM XL C/C++ V16.1.1 (Community Edition)

## Support / Community

Please refer to the manual pages that come with gcli.
You may want to start at `gcli(1)`.
For further questions refer to the issues on Github and Gitlab.

Also, there's an IRC channel #gcli on [Libera.Chat](https://libera.chat/).

Alternatively you may also use the mailing list at
[https://lists.sr.ht/~herrhotzenplotz/gcli-discuss](https://lists.sr.ht/~herrhotzenplotz/gcli-discuss).

## Bugs and contributions

Please report bugs, issues and questions to [~herrhotzenplotz/gcli-discuss@lists.sr.ht](mailto:~herrhotzenplotz/gcli-discuss@lists.sr.ht) or on [GitLab](https://gitlab.com/herrhotzenplotz/gcli).

You can also submit patches using git-send-email or Mercurial patchbomb to [~herrhotzenplotz/gcli-devel@lists.sr.ht](mailto:~herrhotzenplotz/gcli-devl@lists.sr.ht).

## License

BSD-2 CLAUSE (aka. FreeBSD License). Please see the LICENSE file attached.

## Credits

This program makes heavy use of both [libcurl](https://curl.haxx.se/) and [pdjson](https://github.com/skeeto/pdjson).
