include "gcli/gitlab/issues.h";

parser gitlab_user is object of sn_sv select "username" as sv;

parser gitlab_issue_milestone is
object of gcli_issue with
	("title" => milestone as sv);

parser gitlab_issue is
object of gcli_issue with
	("title"             => title as sv,
	 "state"             => state as sv,
	 "description"       => body as sv,
	 "created_at"        => created_at as sv,
	 "iid"               => number as int,
	 "user_notes_count"  => comments as int,
	 "author"            => author as user_sv,
	 "discussion_locked" => locked as bool,
	 "labels"            => labels as array of sn_sv use parse_sv,
	 "assignees"         => assignees as array of gitlab_user
	                        use parse_gitlab_user,
	 "milestone"         => use parse_gitlab_issue_milestone);

parser gitlab_issues is array of gcli_issue use parse_gitlab_issue;