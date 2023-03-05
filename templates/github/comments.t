include "gcli/github/comments.h";

parser github_comment is
object of gcli_comment with
	("created_at" => date as string,
	 "body"       => body as string,
	 "user"       => author as user);

parser github_comments is array of gcli_comment
	use parse_github_comment;