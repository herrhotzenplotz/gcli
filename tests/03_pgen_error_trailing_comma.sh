#!/bin/sh -xe

. $(dirname $0)/setup.sh

TMPFILE=$(mktemp)

cat <<EOF | ( ! ${PGEN} > $TMPFILE ) || fail "parse succeeded"
parser github_issue
is object of gcli_issue
with
   ("foo" => meh as string,
   );
EOF

rm $TMPFILE
