#!/bin/sh

findversion() {
	grep 'GCLI_VERSION' build.sh \
		| head -1 \
		| awk -F\= '{print $2}' \
		| sed -E 's/[[:space:]]*\"([^[:space:]]*)\"[[:space:]]*$/\1/g'
}

VERSION=$(findversion)

DIR=/tmp/gcli-${VERSION}
mkdir -p $DIR

echo "Making BZIP tarball"
git archive --format=tar --prefix=gcli-$VERSION/ v$VERSION \
	| bzip2 -v > $DIR/gcli-$VERSION.tar.bz2
echo "Making XZ tarball"
git archive --format=tar --prefix=gcli-$VERSION/ v$VERSION \
	| xz -v > $DIR/gcli-$VERSION.tar.xz
echo "Making GZIP tarball"
git archive --format=tar --prefix=gcli-$VERSION/ v$VERSION \
	| gzip -v > $DIR/gcli-$VERSION.tar.gz

OLDDIR=$(pwd)
cd $DIR
echo "Calculating SHA256SUMS"
sha256sum *.tar* > SHA256SUMS
cd $OLDDIR

echo "Release Tarballs are at $DIR"
