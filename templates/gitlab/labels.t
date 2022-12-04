include "gcli/gitlab/labels.h";

parser gitlab_label is
object of gcli_label with
	   ("name"        => name as string,
	    "description" => description as string,
	    "colour"      => colour as gitlab_style_colour,
	    "id"          => id as int);

parser gitlab_labels is array of gcli_label use parse_gitlab_label;
