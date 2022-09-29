include "gcli/issues.h";
include "gcli/labels.h";
include "gcli/json_util.h";

parser github_label is object of sn_sv select "name" as sv;

parser github_issue
is object of gcli_issue
with
   ("title"  => title as sv,
	"user"   => author as user_sv,
	"number" => number as int,
	"labels" => labels as array of github_label use parse_github_label);
