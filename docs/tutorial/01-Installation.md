# Installing GCLI

## Through package manager

If you're on FreeBSD you can just install gcli by running the
following command:

    # pkg install gcli

## Compile the source code

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
