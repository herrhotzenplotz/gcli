include "gcli/issues.h";

parser bugzilla_bug_item is
object of gcli_issue with
	("id" => number as id,
	 "summary" => title as sv);

parser bugzilla_bugs is
object of gcli_issue_list with
	("bugs" => issues as array of gcli_issue use parse_bugzilla_bug_item);

