include "gcli/gitlab/releases.h";

parser gitlab_release_asset is
object of gcli_release_asset with
	("url" => url as string);

parser gitlab_release_assets is
object of gcli_release with
	("sources" => assets as array of gcli_release_asset
	              use parse_gitlab_release_asset);

parser gitlab_release is
object of gcli_release with
	("name"             => name as sv,
	 "tag_name"         => id as sv,
	 "description"      => body as sv,
	 "assets"           => use parse_gitlab_release_assets,
	 "author"           => author as user_sv,
	 "created_at"       => date as sv,
	 "upcoming_release" => prerelease as bool);

parser gitlab_releases is
array of gcli_release use parse_gitlab_release;