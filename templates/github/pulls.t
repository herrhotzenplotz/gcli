include "gcli/pulls.h";
include "templates/github/labels.h";

parser github_commit_author_field is
object of gcli_commit with
	("name"  => author as string,
	 "email" => email as string,
	 "date"  => date as string);

parser github_commit_commit_field is
object of gcli_commit with
	("message" => message as string,
	 "author"  => use parse_github_commit_author_field);

parser github_commit is
object of gcli_commit with
	("sha"    => sha as string,
	 "commit" => use parse_github_commit_commit_field);

parser github_commits is array of gcli_commit
	use parse_github_commit;

parser github_pull_head is
object of gcli_pull with
	("sha"   => head_sha as string,
	 "label" => head_label as string);

parser github_branch_label is
object of gcli_pull with
	("label" => base_label as string);

parser github_pull_milestone is
object of gcli_pull with
	("title" => milestone as string);

parser github_pull is
object of gcli_pull with
	("title"         => title as string,
	 "state"         => state as string,
	 "body"          => body as string,
	 "created_at"    => created_at as string,
	 "number"        => number as id,
	 "id"            => id as id,
	 "commits"       => commits as int,
	 "labels"        => labels as array of github_label
	                    use parse_github_label_text,
	 "comments"      => comments as int,
	 "additions"     => additions as int,
	 "deletions"     => deletions as int,
	 "changed_files" => changed_files as int,
	 "merged_at"     => merged as is_string,
	 "mergeable"     => mergeable as bool,
	 "draft"         => draft as bool,
	 "user"          => author as user,
	 "head"          => use parse_github_pull_head,
	 "base"          => use parse_github_branch_label,
	 "milestone"     => use parse_github_pull_milestone);

parser github_pr_merge_message is object of char* select "message" as string;

parser github_pulls is array of gcli_pull
	use parse_github_pull;
