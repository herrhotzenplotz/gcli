# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([gcli],[0.9.13-devel],[nsonack@herrhotzenplotz.de],[gcli],[https://gitlab.com/herrhotzenplotz/gcli/])
AM_INIT_AUTOMAKE([1.0 foreign subdir-objects dist-bzip2 dist-xz -Wall])

dnl Silent by default.
AM_SILENT_RULES([yes])

AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])

AM_PROG_AR

LT_INIT([pic-only])
AC_SUBST([LIBTOOL_DEPS])

AC_PROG_CC([cc clang gcc cl])
AC_LANG([C])
AC_PROG_YACC
AC_PROG_LEX(noyywrap)

PKG_PROG_PKG_CONFIG

dnl Go looking for libcurl
OPT_LIBCURL=check
AC_ARG_WITH([libcurl],
[AS_HELP_STRING([--with-libcurl],
                [Give an alternate path to libcurl.])],
OPT_LIBCURL=$withval
)

AS_IF([test "x$OPT_LIBCURL" = "xno"],
      [AC_MSG_ERROR([--with-libcurl must not be disabled])])
AS_IF([test "x$OPT_LIBCURL" = "xcheck"],
      [PKG_CHECK_MODULES([LIBCURL], [libcurl],,[AC_MSG_ERROR([Could not find libcurl])])
       CFLAGS="$LIBCURL_CFLAGS $CFLAGS"
       LDFLAGS="$LIBCURL_LIBS $LDFLAGS"],
      [CPPFLAGS="-I$OPT_LIBCURL/include $CPPFLAGS"
       LDFLAGS="-L$OPT_LIBCURL/lib $LDFLAGS"])
AC_CHECK_HEADER([curl/curl.h],,[AC_MSG_ERROR([Cannot find libcurl headers])])
AC_CHECK_LIB([curl],[curl_easy_init],,[AC_MSG_ERROR([-lcurl doesn not contain curl_easy_init])])

dnl Go looking for headers that may define getopt_long
AC_CHECK_HEADERS([getopt.h unistd.h])
dnl Other headers that we require
AC_CHECK_HEADERS([string.h signal.h sys/wait.h unistd.h sys/mman.h sys/types.h],
                 ,
                 [AC_MSG_ERROR([Required header not found])])

dnl Check that the function is defined somewhere.
AC_CHECK_FUNC([getopt_long])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
