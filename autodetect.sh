#!/bin/sh

# set -x
info() {
    echo "$*" 1>&2
}

die() {
    info "Config Error: $*"
    exit 1
}

dump() {
    echo "$*" >> config.mk
}

checking() {
    printf -- "Checking $*..." 1>&2
}

checking_result() {
    info $*
}

target_triplet() {
    HOSTOS=`uname | awk '{ print(tolower($0)) }'`
    HOSTCPU=`uname -p | awk '{ print(tolower($0)) }'`

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
            COMPILER_FLAGS="\${CFLAGS} -errfmt=error -erroff=%none -errshort=full -xstrconst -xildoff -xmemalign=8s -xnolibmil -xcode=pic32 -xregs=no%appl -xlibmieee -ftrap=%none -xbuiltin=%none -xunroll=1 -Qy -xdebugformat=dwarf \${CPPFLAGS} -D_POSIX_PTHREAD_SEMANTICS -D_LARGEFILE64_SOURCE -D_TS_ERRNO -D_FILE_OFFSET_BITS=64 \${LDFLAGS} -m64"
            ;;
        *)
            COMPILER_FLAGS="\${CFLAGS} \${CPPFLAGS} \${CFLAGS_${TARGET}}"
            ;;
    esac

    dump "COMPILE_FLAGS=${COMPILER_FLAGS}"
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

    dump "LINK_FLAGS=${LINK_FLAGS} \${LDFLAGS} \${LDFLAGS_${TARGET}}"
    checking_result "ok"
}

c_compiler() {
    checking "C compiler"
    if [ ! "${CC}" ]; then
        for compiler in /opt/developerstudio*/bin/cc /usr/bin/cc /usr/bin/gcc* /usr/bin/clang* /usr/local/bin/gcc*; do
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
}

linker() {
    checking "linker"
    LD=${LD-${CC}}

    [ $LD ] || die "I have no linker"

    dump "LD=${LD}"
    checking_result "${LD}"
}

main() {
    target_triplet
    linker
    compiler_flags
    linker_flags
}

main
