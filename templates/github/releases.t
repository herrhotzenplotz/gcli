include "gcli/releases.h";

parser github_release_asset is
object of gcli_release_asset with
       ("browser_download_url" => url as string,
        "name" => name as string);

parser github_release is
object of gcli_release with
       ("name"       => name as sv,
        "body"       => body as sv,
        "id"         => id as int_to_sv,
        "author"     => author as user_sv,
        "created_at" => date as sv,
        "draft"      => draft as bool,
        "prerelease" => prerelease as bool,
        "assets"     => assets as array of gcli_release_asset use parse_github_release_asset,
        "upload_url" => upload_url as sv,
        "html_url"   => html_url as sv);

parser github_releases is
array of gcli_release use parse_github_release;