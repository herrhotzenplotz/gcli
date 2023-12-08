#!/bin/sh -xe
#
# This script builds the tutorial and then creates a tarball that
# is pulled regularly from the server
#

cd $(dirname $0)

# Build the tutorial
(
	cd tutorial
	./gen.sh
)

# Make a dist directory and copy over files
DISTDIR=$(mktemp -d)
mkdir -p ${DISTDIR}/tutorial
mkdir -p ${DISTDIR}/assets

cp -vp index.html ${DISTDIR}/
cp -vp \
	tutorial/0*.html \
	tutorial/index.html \
	${DISTDIR}/tutorial

cp -vp \
	../screenshot-04.png \
	${DISTDIR}/assets/screenshot.png

tar -c -f - -C ${DISTDIR} \. | xz > website_dist.tar.xz

rm -fr ${DISTDIR}
