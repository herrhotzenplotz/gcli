include "gcli/milestones.h";

parser gitea_milestone is
object of gcli_milestone with
       ("id" => id as int,
        "title" => title as string,
	"created_at" => created_at as string,
	"description" => description as string,
	"state" => state as string,
	"updated_at" => updated_at as string,
	"open_issues" => open_issues as int,
	"closed_issues" => closed_issues as int);

parser gitea_milestones is
array of gcli_milestone use parse_gitea_milestone;
