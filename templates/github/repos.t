include "gcli/github/repos.h";
include "gcli/gitea/repos.h";

parser github_repo is
object of gcli_repo with
	("id"         => id as int,
	 "full_name"  => full_name as sv,
	 "name"       => name as sv,
	 "owner"      => owner as user_sv,
	 "created_at" => date as sv,
	 "visibility" => visibility as sv,
	 "private"    => visibility as gitea_visibility,
	 "fork"       => is_fork as bool);

parser github_repos is array of gcli_repo
	use parse_github_repo;