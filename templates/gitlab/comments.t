include "gcli/gitlab/comments.h";

parser gitlab_comment is
object of gcli_comment with
	   ("created_at" => date as string,
		"body" => body as string,
		"author" => author as user);

parser gitlab_comments is
array of gcli_comment use parse_gitlab_comment;
