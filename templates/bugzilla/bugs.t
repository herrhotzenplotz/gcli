include "gcli/comments.h";
include "gcli/issues.h";
include "gcli/bugzilla/bugs.h";

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

parser bugzilla_comment is
object of gcli_comment with
	("id" => id as id,
	 "text" => body as string,
	 "creation_time" => date as string,
	 "creator" => author as string);

parser bugzilla_comments_internal_skip_first is
object of gcli_comment_list with
	("comments" => use parse_bugzilla_comments_array_skip_first);

parser bugzilla_comments is
object of gcli_comment_list with
	("bugs" => use parse_bugzilla_bug_comments_dictionary_skip_first);
