# Hacking on GCLI

This document gives you hints to get started working with on source
code of [gcli](https://herrhotzenplotz.de/gcli/).

Please note that this document only captures the state of the code at
the some points in time and may be out of date. If you feel like this
is the case please submit bug reports or, even better, provide
patches.

## Building GCLI

We use the GNU Autotools to build GCLI. Using the autotools has a few
advantages:

- Portability across many platforms, even many older ones
- I (Nico) know Autotools fairly well
- Cross-Compilation is easy (though not yet fully supported by gcli)
- Maturity of the tooling due to many years of development
- Very few dependencies

### General workflow

Autotools generate a `configure` script from `configure.ac` and
Makefiles using the `configure` script from `Makefile.am`.

The `configure` script checks the build system for various features
and edge cases and allows you to enable features etc. This is the
place where you can set the C Compiler to be used, flags that should
be passed to it and where libraries should be found.

To generate the configure script you need to invoke autoreconf via the
provided autogen script:

    $ ./autogen.sh

I suggest out-of-tree builds (that is source code is separate from
build output).

For various different build configurations I create different build
directories and one for general debug work:

    $ mkdir build build-sanitized build-32
    $ cd build/

Note: In the following I assume LLVM Clang like compiler options. If
your compiler uses different flags (like e.g. Oracle DeveloperStudio)
please change the options appropriately.

#### Full Debug build

Then you can configure each build directory with appropriate options:

    $ ../configure \
        CC=/usr/bin/cc \
        CFLAGS='-std=iso9899:1999 -pedantic -Wall -Wextra -Wno-misleading-indentation -Werror -g -O0' \
        LDFLAGS='-g' \
        --disable-shared

The above will give you a fully debuggable and build with strict C99
compiler errors. I very much suggest that you use those options while
working on and debugging gcli.

*Note*: The `--disable-shared` is required because if you build a
shared version of libgcli, libtool will replace the gcli binary with a
shell script that alters the dld search path to read the correct
`libgcli.so`. Because of `build/gcli` now not being an ELF
executable but a shell script debuggers can't load gcli properly.

#### Sanitized Builds

I sometimes enable the sanitizer features of the C Compiler to check
for common bugs:

    $ ../configure CC=/usr/bin/cc \
        CFLAGS='-std=iso9899:1999 -pedantic -Wall -Wextra
                -Wno-misleading-indentation -Werror -g -O0
                -fsanitize=address,undefined' \
        LDFLAGS=-g \
        --disable-shared

#### Cross-Compilation

This is not yet fully supported by gcli. However, it is possible to
e.g. build a 32bit version of gcli on a 64bit host OS.

In this example I have a 32bit version of libcurl installed in
/opt/sn. To build gcli against that version you can do something like
the following:

    $ ../configure CC=/usr/bin/cc \
        CFLAGS='-m32 -std=iso9899:1999 -pedantic -Wall -Wextra
        -Wno-misleading-indentation -Werror -g -O0' \
        LDFLAGS='-g -L/opt/sn/lib32' \
        --with-libcurl=/opt/sn

Note: The RPATH will be set automatically by configure. You don't need
to cram it into the LDFLAGS.

## Tests

The test suite depends on [Kyua](https://github.com/jmmv/kyua) and
[libatf-c](https://github.com/jmmv/atf).

Before submitting patches please make sure that your changes pass the
test suite:

    $ make -C build check

If you change the build system also make sure that it passes a distcheck:

    $ make -C build distcheck

# Code Style

Please use the BSD Style conventions for formatting your code. This means:

- Functions return type and name go on separate lines, no mixed code
  and declarations (except in for loops):

        void
        foo(int bar)
        {
            int x, y, z;

            x = bar;

            for (int i = 0; i < 10; ++i)
                z += i;

            return x;
        }

  This allows to search for the implementation of a function through a
  simple `grep -rn '^foo' .`.

- Use struct tags for structs, do not typedef them

        struct foo {
            int bar;
            char const *baz;
        };

        static void
        foodoo(struct foo const *const bar)
        {
        }

- Indent with tabs, align with spaces

  `»` denotes a TAB character, `.` indicates a whitespace:

        void
        foo(struct foo const *thefoo)
        {
        »   if (thefoo)
        »   »   printf("%s: %d\n"
        »   »   .......thefoo->wat,
        »   »   .......thefoo->count);
        }

- Try to have a max of 80 characters per line

  I know we're not using punchcards anymore, however it makes the code
  way more readable.

- Use C99

  Please don't use C11 or even more modern features. Reason being that
  I want gcli to be portable to older platforms where either no modern
  compilers are available or where we have to rely on old gcc versions
  and/or buggy vendor compilers. Notable forbidden features are
  `_Static_assert` and anonymous unions. If you use the compiler flags
  I mentioned above you should get notified by the compiler.

There is a `.editorconfig` included in the source code that should
automatically provide you with all needed
options. [Editorconfig](https://editorconfig.org/#pre-installed) is a
plugin that is available for almost all notable editors out there. I
highly recommend you use it.

# Adding support for new forges

The starting point for adding forges is
[include/gcli/forges.h](include/gcli/forges.h). This file contains the
dispatch table for fetching data from various kinds of forges.

A pointer to the current dispatch table can be retrieved through a
call to `gcli_forge()`. You may have to adjust the routines called by
it to allow for automagic detection as well as overrides on the
command line for your new forge type. You should likely never call
`gcli_forge()` directly when adding a new forge type as there are
various frontend functions available that will do dispatching for the
caller.

## Parsing JSON

When you need to parse JSON Objects into C structs you likely want to
generate that code. Please see the [templates/](templates/) directory
for examples on how to do that. Currently the [PR Parser for
Github](templates/github/pulls.t) can act as an example for all
features available in the code generator.

The code generator is fully documented in [pgen.org](docs/pgen.org).

## Generating JSON

We not only need to parse JSON often, we also need to generate it
on the fly when submitting data to forge APIs.

For this the `gcli_jsongen_` family of functions exist. Since these
have been introduced quite late in the project their use is not
particularly wide-spread. However this may change in the future.

To use these, take a look at the header
[include/gcli/json_gen.h](include/gcli/json_gen.h) and also the use
in [src/gitlab/merge_requests.c](src/gitlab/merge_requests.c).

# User Frontend Features

The gcli command line tool links against libgcli. Through a context
structure it passes information like the forge type and user
credentials into the library.

All code for the command line frontend tool is found in the
[src/cmd/](src/cmd/) directory.

[src/cmd/gcli.c](src/cmd/gcli.c) is the entry point for the command
line tool. In this file you can find the dispatch table for all
subcommands of gcli.

## Subcommands

Subcommand implementations are found in separate C files in the
`src/cmd` subdirectory.

When parsing command line options please use `getopt_long`. Do not
forget to prefix your getopt string with a `+` as we do multiple calls
to `getopt_long` so it needs to reset some internal state.

## Output formatting

Output is usually formatted as a dictionary or a table. For these
cases gcli provides a few convenience functions and data structures.

The relevant header is [gcli/cmd/table.h](include/gcli/cmd/table.h).

Do not use these functions in the library code. It's only supposed
to be used from the command line tool.

### Tables

You can print tables by defining a list of columns first:

```C
gcli_tblcoldef cols[] = {
    { .... },
    { .... },
};
```

For a complete definition look at the header or uses of that interface
in e.g. [src/cmd/issues.c](src/cmd/issues.c).

You can then start adding rows to your table:

```C
gcli_tbl table = gcli_tbl_begin(cols, ARRAY_SIZE(cols));

for (int i = 0; i < list.whatever_size; ++i) {
    gcli_tbl_add_row(table, ...);
}
```

The variadic arguments you need to provide depends on the columns
defined. Most relevant is the flags and type field. Make sure you get
data type sizes correct.

To dump the table to stdout use the following call:

```C
gcli_tbl_end(table);
```

This will print the table and free all resources acquired by calls to
the tbl routines. You may no reuse the handle returned by
`gcli_tbl_begin()` after this call. Instead, call the begin routine
again to obtain a new handle.

### Dictionaries

The dictionary routines act almost the same way as tables except that
you don't define the columns. Instead you obtain a handle through
`gcli_dict_begin` and add entries to the dictionary by calling
`gcli_dict_add` or one of the specialized functions for
strings. `gcli_dict_add` is the most generic of them all and provides
a printf-like format and variadic argument list. You can dump the
dictionary and free resources through a call to `gcli_dict_end`.
