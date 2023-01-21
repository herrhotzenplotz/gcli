include "gcli/milestones.h";

parser github_milestone is
object of gcli_milestone with
	("number" => id as int,
	 "title" => title as string,
	 "created_at" => created_at as string,
	 "state" => state as string,
	 "updated_at" => updated_at as string,
	 "description" => description as string);

parser github_milestones is
array of gcli_milestone use parse_github_milestone;