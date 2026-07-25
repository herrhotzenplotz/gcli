include "templates/github/repos.h";

parser gitea_repo_search_result is
object of struct gcli_repo_list with
	("data" => repos as array of gcli_repo use parse_github_repo);
