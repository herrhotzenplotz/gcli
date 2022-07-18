#!/opt/schily/bin/bosh

die() {
	printf -- "error: %s\n" "$*" 1>&2
	exit 1
}

info() {
	printf -- "info: %s\n" "$*" 1>&2
}

findversion() {
	grep '^GCLI_VERSION' Makefile \
		 | head -1 \
		 | awk -F\= '{print $2}' \
		 | sed -E 's/[[:space:]]*([^[:space:]]*)[[:space:]]*$/\1/g'
}

stage() {
	info "Creating staging directory..."
	mkdir -p ${STAGEDIR}

	find . -type f \
		 ! -path \./\.git/\* \
		 ! -name \.gitignore \
		 ! -name \*.o \
		 ! -name \*.d \
		 ! -name \*~ \
		 ! -name gcli \
		 ! -name \*.a \
		 ! -name todo.org \
		 ! -name config.mk \
		 ! -name TAGS \
		 ! -name \*.gz \
		 ! -path \./tools/\* \
		 ! -name y.tab.h \
		 -exec cp {} ${STAGEDIR} \;
}



NAMEBASE="gcli-`findversion`"
TARFBASE="${NAMEBASE}.tar"
STAGEDIR="/tmp/${NAMEBASE}/"

if [ ! -f Makefile ]; then
	die "Please change to the source top"
fi

pushd .. > /dev/null 2>&1

info "Staging into ${STAGEDIR} ..."
pushd gcli > /dev/null 2>&1
stage
popd > /dev/null 2>&1

info "Creating base tarball ${TARFBASE} ..."
pushd /tmp > /dev/null 2>&1

star -c -f "${TARFBASE}" "${NAMEBASE}"

info "Compressing tarballs..."
xz < "${TARFBASE}" > "${TARFBASE}".xz
gzip < "${TARFBASE}" > "${TARFBASE}".gz
bzip2 < "${TARFBASE}" > "${TARFBASE}".bz2

info "Shuffling around output files..."

rm "${TARFBASE}"
rm -rf "${STAGEDIR}"
mkdir -p "${STAGEDIR}"

mv "${TARFBASE}".* "${STAGEDIR}"

popd > /dev/null 2>&1

info "Tarballs are at ${STAGEDIR}"

popd > /dev/null 2>&1
