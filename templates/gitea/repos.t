include "gcli/repos.h";

parser gitea_repo is
object of struct gcli_repo with
	("id"         => id as id,
	 "full_name"  => full_name as string,
	 "name"       => name as string,
	 "owner"      => owner as user,
	 "created_at" => date as iso8601_time,
	 "private"    => visibility as gitea_visibility,
	 "fork"       => is_fork as bool);

parser gitea_repos is
array of struct gcli_repo use parse_gitea_repo;

parser gitea_repo_search_result is
object of struct gcli_repo_list with
	("data" => repos as array of gcli_repo use parse_gitea_repo);
