#!/bin/sh
# Set the following options:
# -e: Exit immediately if any command exits with a non-zero status (error).
# -u: Treat unset variables as errors, causing the script to exit.
set -eu

#
# Static Site generator for the tutorial.
#
#  You will need cmark for this to work.
#

header() {
	TITLE="${1}"
	PREVURL="${2-}"
	NEXTURL="${3-}"
	sed -e "s/{{TITLE_PLACEHOLDER}}/${TITLE}/g" \
	    -e "s/{{PREVURL}}/${PREVURL}/g" \
	    -e "s/{{NEXURL}}/${NEXTURL}/g" top.html
}

footer() {
	cat footer.html
}

pagination() {
	PREVDOC="$1"
	PREVTITLE="$2"
	NEXTDOC="$3"
	NEXTTITLE="$4"

	echo "<nav style=text-align:right>"
	if [ -n "${PREVDOC}" ]; then
		echo "<a href=\"${PREVDOC}\" title=\"${PREVTITLE}\">⇐ Previous</a>"
	fi

	echo "<a href=\"index.html\">Table of contents</a>"

	if [ -n "${NEXTDOC}" ]; then
		echo "<a href=\"${NEXTDOC}\" title=\"${NEXTTITLE}\">Next ⇒</a>"
	fi
	echo "</nav>"
}

genindex() {
	header "Index"

	cat <<EOF
	<h1>A GCLI Tutorial</h1>
	<p>This document is aimed at those who are new to gcli and want get
	started using it.</p>

	<h2>Table of contents</h2>
EOF

	echo "<ol>"

	awk -F\\t '{printf "<li><a href=\"./%s\">%s</a></li>\n", $1, $2}' < toc

	echo "</ol>"

	footer
}

genpage() {
	PAGETITLE="$1"
	PAGEMDFILE="$2"
	PREVDOC="$3"
	PREVTITLE="$4"
	NEXTDOC="$5"
	NEXTTITLE="$6"

	header "${PAGETITLE}" "${PREVDOC}" "${NEXTDOC}"

	pagination "${PREVDOC}" "${PREVTITLE}" "${NEXTDOC}" "${NEXTTITLE}"
	echo "<hr />"


	cmark -t html < "${PAGEMDFILE}"
	echo "<br />"
	echo "<hr />"

	pagination "${PREVDOC}" "${PREVTITLE}" "${NEXTDOC}" "${NEXTTITLE}"

	footer
}

prevhtmldoc=""
prevtitlename=""
while IFS="$(printf '\t')" read -r htmldoc title; do
	mddoc="${htmldoc%.html}.md"


	read -r nexthtmldoc nexttitle <<EOF
$(awk -F'\t' -v current="$htmldoc" 'BEGIN{flag=0} {if(flag==1){print $1 "\t" $2; exit} if($1==current){flag=1}}' toc)
EOF

	if [ -n "$prevhtmldoc" ]; then
	prevtitlename=$(awk -F'\t' -v current="$prevhtmldoc" '{if($1==current){print $2; exit}}' toc)
	fi

	# Generating the pages
	genpage "${title}" "${mddoc}" "${prevhtmldoc}" "${prevtitlename}" "${nexthtmldoc}" "${nexttitle}" > "${htmldoc}"

	# Update the previous document filename and title for the next iteration
	prevhtmldoc="$htmldoc"
	prevtitlename="$title"
done < toc

genindex > index.html

