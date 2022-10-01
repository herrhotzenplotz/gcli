include "gcli/github/checks.h";

parser github_check is
object of gcli_github_check with
	   ("name" => name as string,
		"status" => status as string,
		"conclusion" => conclusion as string,
		"started_at" => started_at as string,
		"completed_at" => completed_at as string,
		"id" => id as int);

parser github_checks is
object of gcli_github_checks with
	   ("check_runs" => checks as array of gcli_github_check use parse_github_check);