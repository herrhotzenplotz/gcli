include "gcli/forks.h";

parser github_fork is
object of gcli_fork with
	("full_name"   => full_name as sv,
	 "owner"       => owner as user_sv,
	 "created_at"  => date as sv,
	 "forks_count" => forks as int);

parser github_forks is array of gcli_fork
	use parse_github_fork;