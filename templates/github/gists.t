include "gcli/json_util.h";
include "gcli/github/gists.h";

parser github_gist_file is
object of gcli_gist_file with
	("filename" => filename as string,
	 "language" => language as string,
	 "raw_url"  => url as string,
	 "size"     => size as size_t,
	 "type"     => type as string);

parser github_gist is
object of gcli_gist with
	("owner"        => owner as user,
	 "html_url"     => url as string,
	 "id"           => id as string,
	 "created_at"   => date as string,
	 "git_pull_url" => git_pull_url as string,
	 "description"  => description as string,
	 "files"        => use parse_github_gist_files_idiot_hack);

parser github_gists is array of gcli_gist
	use parse_github_gist;
