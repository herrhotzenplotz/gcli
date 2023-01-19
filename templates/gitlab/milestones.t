include "gcli/milestones.h";

parser gitlab_milestone is
object of gcli_milestone with
	("title" => title as string,
	 "id" => id as int,
	 "state" => state as string,
	 "created_at" => created_at as string);

parser gitlab_milestones is array of gcli_milestone use parse_gitlab_milestone;
