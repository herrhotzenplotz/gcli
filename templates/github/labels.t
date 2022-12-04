include "gcli/github/labels.h";

parser github_label_text is object of sn_sv select "name" as sv;

parser github_label is
object of gcli_label with
	   ("id" => id as int,
		"name" => name as string,
		"description" => description as string,
		"colour" => colour as github_style_colour);

parser github_labels is array of gcli_label use parse_github_label;