include "gcli/gitlab/releases.h";

parser gitlab_release_asset is object of sn_sv select "url" as sv;

parser gitlab_release_assets is
object of gcli_release with
       ("sources" => asset_urls as array of sn_sv use parse_gitlab_release_asset);

parser gitlab_release is
object of gcli_release with
       ("name" => name as sv,
        "tag_name" => id as sv,
        "description" => body as sv,
        "assets" => use parse_gitlab_release_assets,
        "author" => author as user_sv,
        "created_at" => date as sv);

parser gitlab_releases is
array of gcli_release use parse_gitlab_release;