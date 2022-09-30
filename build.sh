#!/bin/sh
# -*- shell-script -*-

########################################################################
# Helpers
########################################################################
info()
{
	printf -- "$*\n" 1>&2
}

die()
{
	info "error: $*"
	exit 1
}

########################################################################
# Configuration Procedures
########################################################################
find_program()
{
	PROGNAME=$1

	while :; do
		shift
		if [ x$1 = x ]; then
			break
		fi

		PROGPATH=`which $1 2>/dev/null`
		if [ x$PROGPATH != x ]; then
			echo "$PROGPATH"
			return
		fi
	done
	exit 1
}

#####################################################
# OS Details
#####################################################
target_triplet()
{
	HOSTOS=`uname | awk '{ print(tolower($0)) }'`
	HOSTCPU=`uname -p | awk '{ print(tolower($0)) }'`

	# Check correct path on Slowlaris
	if [ "${HOSTOS}" = "SunOS" ]; then
		if [ `which awk 2>&1` = "/usr/xpg4/bin/awk" ]; then
			true # this is good
		else
			die "Check your PATH. We need /usr/xpg4/bin before /usr/bin. At least you've got a silly awk that I don't wanna support."
		fi
	fi

	# Because GNU messes with us
	if [ "${HOSTCPU}" = "unknown" ]; then
		HOSTCPU=`uname -m | awk '{ print(tolower($0)) }'`
	fi

	# CPU substitutions
	case "${HOSTCPU}" in
		x86_64|x64)
			HOSTCPU=amd64
			;;
	esac

	TARGET="${HOSTCPU}-${HOSTOS}-${CCNAME}"
	info "Target is $TARGET"
}

########################################################
# Find C compiler details
########################################################
c_compiler()
{
	info "Searching for usable C Compiler"
	if [ ! "${CC}" ]; then
		for compiler in /opt/developerstudio*/bin/cc /usr/bin/cc /usr/bin/gcc* /usr/bin/clang* /usr/local/bin/gcc* /usr/bin/tcc /usr/local/bin/tcc; do
			which ${compiler} 2>/dev/null >/dev/null
			[ $? -eq 0 ] || continue

			CC=${compiler}
			break
		done
	fi

	[ ${CC} ] || die "I have no C Compiler"

	# Here is a hack to detect some C compilers I know and use
	# Sun Studio
	foo=`${CC} -V 2>&1 | grep "Studio"`
	if [ $? -eq 0 ] && [ "${foo}" ]; then
		CCNAME="sunstudio"
		info "Found SunStudio Compiler"
		return
	fi

	# GCC
	foo=`${CC} -v 2>&1 | grep "gcc version"`
	if [ $? -eq 0 ] && [ "${foo}" ]; then
		CCNAME="gcc"
		info "Found GNU C Compiler"
		return
	fi

	# Clang
	foo=`${CC} -v 2>&1 | grep "clang version"`
	if [ $? -eq 0 ] && [ "${foo}" ]; then
		CCNAME="clang"
		info "Found LLVM Clang"
		return
	fi

	# tcc
	foo=`${CC} -v 2>&1 | grep "tcc version"`
	if [ $? -eq 0 ] && [ "${foo}" ]; then
		info "Found TCC"
		die "TCC cannot be supported due to both its lack of TLS and failures to properly compile the standard library of some BSDs."
	fi
}

compiler_flags()
{
	info "Checking C Compiler Flags"

	if [ $CCNAME = gcc ] || [ $CCNAME = clang ]; then
		[ "x$CFLAGS" != "x" ] || CFLAGS="-std=iso9899:1999 -g -O0"
	elif [ $CCNAME = sunstudio ]; then
		[ "x$CFLAGS" != "x" ] || CFLAGS="-std=iso9899:1999 -g -xO0"
	fi

	[ "x$CPPFLAGS" != "x" ] || CPPFLAGS="-D_XOPEN_SOURCE=600"
}

gcli_version()
{
	GCLI_VERSION="0.9.8-beta"
	CPPFLAGS="${CPPFLAGS} -DGCLI_VERSION_STRING=\"${GCLI_VERSION}\""
}

find_curl()
{
	info "Searching for libcurl"

	if [ "$HOSTOS" = "sunos" ]; then
		for path in /opt/csw/ /opt/bw /usr/local /usr; do
			if [ -f ${path}/lib/libcurl* ]; then
				info "Found libcurl under prefix $path"
				LDFLAGS="$CFLAGS -L${path}/lib -R${path}/lib -lcurl"
				CFLAGS="$CFLAGS -I${path}/include"
				return
			fi
		done
		info "libcurl not found. please make sure your environment allows compiling against it."
	fi

	PKG_CONFIG=${PKG_CONFIG-`which pkg-config 2>/dev/null`}
	if [ "x$PKG_CONFIG" = "x" ]; then
		info "pkg-config not found. Either set PKG_CONFIG or adjust CFLAGS and LDFLAGS for libcurl"
	else
		CFLAGS="$CFLAGS `${PKG_CONFIG} --cflags libcurl`" || die "pkg-config failed"
		LDFLAGS="$LDFLAGS `${PKG_CONFIG} --libs libcurl`" || die "pkg-config failed"
	fi
}

configure()
{
	[ "x$YACC" != "x" ] || YACC=`find_program yacc yacc byacc bison` || die "Could not find yacc"
	[ "x$LEX" != "x" ] || LEX=`find_program lex flex lex` || die "Could not find lex"
	[ "x$RM" != "x" ] || RM=`find_program rm rm` || die "Could not find rm"
	[ "x$AR" != "x" ] || AR=`find_program ar ar` || die "Could not find ar"

	c_compiler
	compiler_flags
	target_triplet

	gcli_version

	find_curl

	case $YACC in
		*bison)
			YFLAGS="$YFLAGS -y"
			;;
	esac

	info "Configuration Summary"
	info " CC       = $CC"
	info " CFLAGS   = $CFLAGS"
	info " CPPFLAGS = $CPPFLAGS"
	info " LEX      = $LEX"
	info " YACC     = $YACC"
	info " YFLAGS   = $YFLAGS"
}

########################################################################
# Basic Build procedures
########################################################################
pgen_cmd()
{
	info "    $*"
	$* || die "pgen build command failed"
}

build_pgen()
{
	info "Building pgen"
	pgen_cmd ${YACC} -d ${YFLAGS} src/pgen/parser.y
	case $HOSTOS in
		sunos)
			if [ -x /opt/bw/bin/m4 ]; then
				M4=/opt/bw/bin/m4
				export M4
				pgen_cmd ${LEX} ${LFLAGS} src/pgen/lexer.l
			else
				pgen_cmd ${LEX} ${LFLAGS} src/pgen/lexer.l
			fi
			;;
		*)
			pgen_cmd ${LEX} ${LFLAGS} src/pgen/lexer.l
			;;
	esac
	pgen_cmd \
		${CC} ${CFLAGS} ${CPPFLAGS} -o pgen \
		-I. \
		-Iinclude \
		y.tab.c \
		lex.yy.c \
		src/pgen/dump_c.c \
		src/pgen/dump_h.c \
		src/pgen/dump_plain.c \
		${LDFLAGS}
	${RM} -f lex.yy.c y.tab.h y.tab.c
}

compile()
{
	info " > Compiling $1..."
	CCMD="$CC $CFLAGS $CPPFLAGS -Igen -Iinclude -Ithirdparty -c -o ${1%.c}.o $1"
	info "   $CCMD"
	$CCMD || die "Compilation command failed"
}

transpile_c()
{
	info " > Transpiling $1... into C"
	TCMD="./pgen -tc -o ${1%.t}.c $1"
	info "   $TCMD"
	$TCMD || die "pgen command failed"
}

transpile_h()
{
	info " > Generating C Header from $1..."
	TCMD="./pgen -th -o gen/${1%.t}.h $1"
	info "   $TCMD"
	$TCMD || die "pgen command failed"
}

########################################################################
# Basic Build procedures
########################################################################
build()
{
	configure
	build_pgen

	GENDIRS="gen/templates/github gen/templates/gitlab gen/templates/gitea"
	for DIR in $GENDIRS; do
		[ -d $DIR ] || mkdir -p $DIR
	done

	for TFILE in $TEMPLATES; do
		transpile_c $TFILE
		transpile_h $TFILE
		SRCS="${TFILE%.t}.c ${SRCS}"
	done

	for CFILE in $SRCS; do
		compile $CFILE
	done

	info " > Archiving libgcli.a"
	ARCMD="$AR -rc libgcli.a `for SRC in $SRCS; do [ $SRC = src/gcli.c ] || printf \"${SRC%.c}.o \"; done`"
	info "    $ARCMD"
	$ARCMD || die "Archiver command failed"

	info " > Linking gcli"
	LCMD="$CC $CFLAGS $CPPFLAGS -o gcli src/gcli.o libgcli.a ${LDFLAGS}"
	info "   $LCMD"
	$LCMD || die "Linker command failed"
}

SRCS="src/*.c src/github/*.c src/gitlab/*.c src/gitea/*.c thirdparty/pdjson/pdjson.c thirdparty/sn/sn.c"
TEMPLATES="templates/github/*.t"

build_test_program()
{
	info " > Building $1"
	compile $1.c
	info " > Linking $1"
	LCMD="$CC $CFLAGS $CPPFLAGS -o $1 $1.o libgcli.a $LDFLAGS -latf-c"
	info "    $LCMD"
	$LCMD || die "Linker command failed"
}

build_test_programs()
{
	info "Building test programs"
	build_test_program tests/github-tests
	build_test_program tests/json-escape
	build_test_program tests/url-encode
	build_test_program tests/pgen-tests
}

# Start it!

case $1 in
	build|"")
		build
		;;
	check)
		build
		build_test_programs
		cd tests
		kyua test || die "Tests failed"
		cd ..
		;;
	*)
		die "unknown subcommand"
		;;
esac
