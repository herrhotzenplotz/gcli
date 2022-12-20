include "gcli/json_util.h";
include "gcli/github/gists.h";

parser github_gist_file is
object of gcli_gist_file with
	("filename" => filename as sv,
	 "language" => language as sv,
	 "raw_url"  => url as sv,
	 "size"     => size as int,
	 "type"     => type as sv);

parser github_gist is
object of gcli_gist with
	("owner"        => owner as user_sv,
	 "html_url"     => url as sv,
	 "id"           => id as sv,
	 "created_at"   => date as sv,
	 "git_pull_url" => git_pull_url as sv,
	 "description"  => description as sv,
	 "files"        => use parse_github_gist_files_idiot_hack);

parser github_gists is array of gcli_gist
	use parse_github_gist;