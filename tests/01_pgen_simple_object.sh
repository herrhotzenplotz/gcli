#!/bin/sh -ex

. $(dirname $0)/setup.sh

TMPFILE=$(mktemp)
cat <<EOF | ${PGEN} > $TMPFILE || fail "parse failed"
parser github_issue
is object of gcli_issue
with
   ("title"  => title as sv,
	"user"   => user as sv use parse_user,
	"id"     => id as int,
	"labels" => labels as array of github_label use parse_label);
EOF

# Validate entries
diff -u $TMPFILE ${srcdir}/tests/samples/pgen_simple_object.dump \
    || fail "unexpected parser dump"

rm -f $TMPFILE
