include "gcli/issues.h";

parser bugzilla_bug_creator is
object of gcli_issue with
	("real_name" => author as string);

parser bugzilla_bug_item is
object of gcli_issue with
	("id" => number as id,
	 "summary" => title as string,
	 "creation_time" => created_at as string,
	 "creator_detail" => use parse_bugzilla_bug_creator,
	 "status" => state as string);

parser bugzilla_bugs is
object of gcli_issue_list with
	("bugs" => issues as array of gcli_issue use parse_bugzilla_bug_item);

