include "gcli/gitlab/repos.h";

parser gitlab_repo is
object of gcli_repo with
	("path_with_namespace" => full_name as sv,
	 "name"                => name as sv,
	 "owner"               => owner as user_sv,
	 "created_at"          => date as sv,
	 "visibility"          => visibility as sv,
	 "fork"                => is_fork as bool,
	 "id"                  => id as id);

parser gitlab_repos is
array of gcli_repo use parse_gitlab_repo;
