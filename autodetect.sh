#!/bin/sh

# set -x
info() {
	printf -- "$*\n" 1>&2
}

die() {
	info "\nConfig Error: $*"
	exit 1
}

dump() {
	echo "$*" >> config.mk
}

checking() {
	printf -- "   > Checking $*..." 1>&2
}

checking_result() {
	info $*
}

target_triplet() {
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

	c_compiler

	HOSTCC=${CC}

	checking "Target Triplet"
	TARGET="${HOSTCPU}-${HOSTOS}-${CCNAME}"
	checking_result "${TARGET}"

	dump "CC=${CC}"
	dump "TARGET=\"${TARGET}\""
}

compiler_flags() {

	checking "compilation flags"

	case "${CCNAME}" in
		sunstudio)
			COMPILER_FLAGS="\${CFLAGS} \${CFLAGS_${TARGET}} -errfmt=error -erroff=%none -errshort=full -xstrconst -xildoff -xmemalign=8s -xnolibmil -xcode=pic32 -xregs=no%appl -xlibmieee -ftrap=%none -xbuiltin=%none -xunroll=1 -Qy -xdebugformat=dwarf \${CPPFLAGS} -D_POSIX_PTHREAD_SEMANTICS -D_LARGEFILE64_SOURCE -D_TS_ERRNO -D_FILE_OFFSET_BITS=64 \${LDFLAGS} -m64"
			MKDEPS_FLAGS="\${COMPILE_FLAGS} -xM1"
			;;
		*)
			COMPILER_FLAGS="\${CFLAGS} \${CPPFLAGS} \${CFLAGS_${TARGET}}"
			MKDEPS_FLAGS="\${COMPILE_FLAGS} -MM -MT \${@:.d=.o}"
			;;
	esac

	dump "COMPILE_FLAGS=	${COMPILER_FLAGS}"
	dump "COMPILE_FLAGS	+=	\${\${@}_CFLAGS}"
	dump "COMPILE_FLAGS	+=	\${LIB_CFLAGS}"
	dump "COMPILE_FLAGS	+=	\${CFLAGS_${CCNAME}}"
	dump "COMPILE_FLAGS	+=	\${CFLAGS_${HOSTOS}}"
	dump "COMPILE_FLAGS	+=	\${CFLAGS_${HOSTCPU}}"
	dump "COMPILE_FLAGS	+=	\${CFLAGS_${HOSTOS}-${CCNAME}}"
	dump "COMPILE_FLAGS	+=	\${CFLAGS_${HOSTOS}-${HOSTCPU}}"
	dump "MKDEPS_FLAGS=${MKDEPS_FLAGS}"
	checking_result "ok"
}

linker_flags() {
	checking "link stage flags"

	case "${CCNAME}" in
		sunstudio)
			LINK_FLAGS="\${COMPILE_FLAGS}"
			;;
		*)
			LINK_FLAGS="\${CFLAGS} \${CPPFLAGS}"
			;;
	esac

	dump "LINK_FLAGS=	${LINK_FLAGS}"
	dump "LINK_FLAGS	+=	\${LDFLAGS}"
	dump "LINK_FLAGS	+=	\${LDFLAGS_${TARGET}}"
	dump "LINK_FLAGS	+=	\${LIB_LDFLAGS}"
	dump "LINK_FLAGS	+=	\${\${@}_LDFLAGS}"
	dump "LINK_FLAGS	+=	\${LDFLAGS_${CCNAME}}"
	dump "LINK_FLAGS	+=	\${LDFLAGS_${HOSTOS}}"
	dump "LINK_FLAGS	+=	\${LDFLAGS_${HOSTCPU}}"
	dump "LINK_FLAGS	+=	\${LDFLAGS_${HOSTOS}-${CCNAME}}"
	dump "LINK_FLAGS	+=	\${LDFLAGS_${HOSTOS}-${HOSTCPU}}"
	checking_result "ok"
}

c_compiler() {
	checking "C compiler"
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
		checking_result "${CC} is ${CCNAME}"
		return
	fi

	# GCC
	foo=`${CC} -v 2>&1 | grep "gcc version"`
	if [ $? -eq 0 ] && [ "${foo}" ]; then
		CCNAME="gcc"
		checking_result "${CC} is ${CCNAME}"
		return
	fi

	# Clang
	foo=`${CC} -v 2>&1 | grep "clang version"`
	if [ $? -eq 0 ] && [ "${foo}" ]; then
		CCNAME="clang"
		checking_result "${CC} is ${CCNAME}"
		return
	fi

	# tcc
	foo=`${CC} -v 2>&1 | grep "tcc version"`
	if [ $? -eq 0 ] && [ "${foo}" ]; then
		die "TCC cannot be supported due to both its lack of TLS and failures to properly compile the standard library of some BSDs."
	fi

}

linker() {
	checking "linker"
	LD=${LD-${CC}}

	[ $LD ] || die "I have no linker"

	dump "LD=${LD}"
	checking_result "${LD}"
}

PKG_CONFIG=${PKG_CONFIG-`which pkg-config 2>/dev/null`}
[ ${PKG_CONFIG} ] || die "Cannot find pkg-config"

check_library() {
	checking "for ${1}"

	cflags=`${PKG_CONFIG} --cflags ${1}`
	ldflags=`${PKG_CONFIG} --libs ${1}`

	[ $? -eq 0 ] || die "Cannot find ${1}"

	dump "LIB_CFLAGS	+=	${cflags}"
	dump "LIB_LDFLAGS	+=	${ldflags}"

	checking_result "found"
}

warn_gnu_make() {
	barf="`${MAKE} -v 2>&1 | grep GNU`"
	if [ "${barf}" != "" ]; then
		info "/!\\ You appear to be using GNU make to build this software."
		info "/!\\ In case you just saw a whole bunch of compiler errors,"
		info "/!\\ this is because of a buggy make implementation found in GNU."
		info "/!\\ PLEASE USE A BETTER MAKE INSTEAD."
	fi
}

main() {
	warn_gnu_make
	target_triplet
	linker
	compiler_flags
	linker_flags

	# Provide -s flag because GNU make is behaving abnormally yet
	# again and prints out that it is going to do a recursion even tho
	# the .SILENT target is defined...This does not break bmake and
	# smake.
	for lib in `${MAKE} -s snmk-libdeps`; do
		check_library "${lib}"
	done
}

MAKE="${1}"

main
