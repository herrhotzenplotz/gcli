#!/bin/sh
#

command -v git > /dev/null 2>&1 || (echo "error: you need git to run this script" && exit 1)

findversion() {
	eval $(grep PACKAGE_VERSION $(dirname $0)/../configure | sed 1q)
	echo $PACKAGE_VERSION
}

VERSION=$(findversion)

DIR=dist/gcli-${VERSION}
mkdir -p $DIR

if [ -d .got ]; then
	repodir=$(got info | grep '^repository' | cut -d: -f2 | xargs)
	head=$(got br)
else
	repodir=$(git rev-parse --show-toplevel)/.git
	head=@
fi

echo "Making BZIP tarball"
git --git-dir="${repodir}" archive --format=tar --prefix=gcli-$VERSION/ $head \
	| bzip2 -v > $DIR/gcli-$VERSION.tar.bz2
echo "Making XZ tarball"
git --git-dir="${repodir}" archive --format=tar --prefix=gcli-$VERSION/ $head \
	| xz -v > $DIR/gcli-$VERSION.tar.xz
echo "Making GZIP tarball"
git --git-dir="${repodir}" archive --format=tar --prefix=gcli-$VERSION/ $head \
	| gzip -v > $DIR/gcli-$VERSION.tar.gz

(
	cd $DIR
	echo "Calculating SHA256SUMS"
	sha256sum *.tar* > SHA256SUMS
)

echo "Release Tarballs are at $DIR"
