include "gcli/forks.h";

parser github_fork is
object of gcli_fork with
	("full_name"   => full_name as string,
	 "owner"       => owner as user,
	 "created_at"  => date as string,
	 "forks_count" => forks as int);

parser github_forks is array of gcli_fork
	use parse_github_fork;
