include "gcli/github/repos.h";
include "gcli/gitea/repos.h";

parser github_repo is
object of gcli_repo with
	("id"         => id as id,
	 "full_name"  => full_name as string,
	 "name"       => name as string,
	 "owner"      => owner as user,
	 "created_at" => date as string,
	 "visibility" => visibility as string,
	 "private"    => visibility as gitea_visibility,
	 "fork"       => is_fork as bool);

parser github_repos is array of gcli_repo
	use parse_github_repo;
