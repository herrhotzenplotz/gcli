include "gcli/gitlab/forks.h";

parser gitlab_fork_namespace is
object of gcli_fork with
	   ("full_path" => owner as sv);

parser gitlab_fork is
object of gcli_fork with
	   ("path_with_namespace" => full_name as sv,
		"namespace" => use parse_gitlab_fork_namespace,
		"created_at" => date as sv,
		"forks_count" => forks as int);

parser gitlab_forks is
array of gcli_fork use parse_gitlab_fork;
