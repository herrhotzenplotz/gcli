include "gcli/gitlab/merge_requests.h";

parser gitlab_mr_milestone is
object of gcli_pull with
	("title" => milestone as string);

parser gitlab_mr_head_pipeline is
object of gcli_pull with
	("id"       => head_pipeline_id as int,
	 "coverage" => coverage as string);

parser gitlab_reviewer is object of char* select "username" as string;

parser gitlab_diff_refs is
object of gcli_pull with
	("base_sha" => base_sha as string,
	 "head_sha" => head_sha as string);

parser gitlab_mr is
object of gcli_pull with
	("title"            => title as string,
	 "state"            => state as string,
	 "description"      => body as string,
	 "created_at"       => created_at as string,
	 "iid"              => number as id,
	 "id"               => id as id,
	 "labels"           => labels as array of sn_sv use parse_sv,
	 "user_notes_count" => comments as int,
	 "merge_status"     => mergeable as gitlab_can_be_merged,
	 "draft"            => draft as bool,
	 "author"           => author as user,
	 "source_branch"    => head_label as string,
	 "target_branch"    => base_label as string,
	 "milestone"        => use parse_gitlab_mr_milestone,
	 "head_pipeline"    => use parse_gitlab_mr_head_pipeline,
	 "reviewers"        => reviewers as array of char* use parse_gitlab_reviewer,
	 "diff_refs"        => use parse_gitlab_diff_refs);

parser gitlab_mrs is array of gcli_pull use parse_gitlab_mr;

parser gitlab_commit is
object of gcli_commit with
	("short_id"     => sha as string,
	 "id"           => long_sha as string,
	 "title"        => message as string,
	 "created_at"   => date as string,
	 "author_name"  => author as string,
	 "author_email" => email as string);

parser gitlab_commits is array of gcli_commit use parse_gitlab_commit;

parser gitlab_reviewer_id is object of gcli_id select "id" as id;

parser gitlab_reviewer_ids is
object of gitlab_reviewer_id_list with
	("reviewers" => reviewers as array of gcli_id use parse_gitlab_reviewer_id);

parser gitlab_diff is
object of gitlab_diff with
	("diff" => diff as string,
	 "new_path" => new_path as string,
	 "old_path" => old_path as string,
	 "a_mode" => a_mode as string,
	 "b_mode" => b_mode as string,
	 "new_file" => new_file as bool,
	 "renamed_file" => renamed_file as bool,
	 "deleted_file" => deleted_file as bool);

parser gitlab_diffs is
array of gitlab_diff use parse_gitlab_diff;
