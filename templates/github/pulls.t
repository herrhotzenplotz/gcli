include "gcli/pulls.h";
include "gcli/json_util.h";

parser github_pull is
object of gcli_pull with
	   ("title" => title as string,
		"state" => state as string,
		"number" => number as int,
		"id" => id as int,
		"merged_at" => merged as is_string,
		"user" => creator as user);

parser github_pulls is
array of gcli_pull use parse_github_pull;
