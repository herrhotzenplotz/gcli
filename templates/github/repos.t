include "gcli/github/repos.h";

parser github_repo is
object of struct gcli_repo with
	("id"         => id as id,
	 "full_name"  => full_name as string,
	 "name"       => name as string,
	 "owner"      => owner as user,
	 "created_at" => date as iso8601_time,
	 "visibility" => visibility as string,
	 "fork"       => is_fork as bool);

parser github_repos is array of struct gcli_repo
	use parse_github_repo;

parser github_repo_search is
object of  struct gcli_repo_list with
	("items" => repos as array of gcli_repo use parse_github_repo);
