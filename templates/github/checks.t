include "gcli/json_util.h";
include "gcli/github/checks.h";

parser github_check is
object of gcli_github_check with
	   ("name" => name as string,
		"status" => status as string,
		"conclusions" => conclusion as string,
		"started_at" => started_at as string,
		"completed_at" => completed_at as string,
		"id" => id as int);
