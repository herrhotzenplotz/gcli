include "gcli/issues.h";
include "gcli/labels.h";
include "gcli/json_util.h";

include "templates/github/labels.h";

parser github_assignee is object of sn_sv select "name" as sv;

parser github_issue
is object of gcli_issue
with
   ("title"  => title as sv,
	"state" => state as sv,
	"body" => body as sv,
	"created_at" => created_at as sv,
	"number" => number as int,
	"comments" => comments as int,
	"user"   => author as user_sv,
	"locked" => locked as bool,
	"labels" => labels as array of github_label use parse_github_label_text,
	"assignees" => assignees as array of github_assignees use parse_github_assignee);

parser github_issues is array of gcli_issue use parse_github_issue;
