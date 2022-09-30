include "gcli/releases.h";
include "gcli/json_util.h";

parser github_release is
object of gcli_release with
	   ("name" => name as sv,
		"id"   => id as int_to_sv,
		"tarball_url" => tarball_url as sv,
		"author" => author as user_sv,
		"created_at" => date as sv,
		"draft" => draft as bool,
		"prerelease" => prerelease as bool,
		"upload_url" => upload_url as sv,
		"html_url" => html_url as sv);

parser github_releases is
array of gcli_release use parse_github_release;