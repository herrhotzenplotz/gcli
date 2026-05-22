include "gcli/github/checks.h";

parser github_check is
object of struct gcli_job with
	("name"         => name as string,
	 "status"       => status as string,
	 "started_at"   => started_at as iso8601_time,
	 "completed_at" => finished_at as iso8601_time,
	 "id"           => id as id);

parser github_check_runs is
object of struct gcli_job_list with
	("check_runs" => jobs as array of gcli_job use parse_github_check);

parser github_checksuite is
object of struct gcli_pipeline with
	("id"          => id as id,
	 "status"      => status as string,
	 "created_at"  => created_at as iso8601_time,
	 "updated_at"  => updated_at as iso8601_time,
	 "head_sha"    => sha as string,
	 "head_branch" => ref as string);

parser github_checksuites is
object of struct gcli_pipeline_list with
	("check_suites" => pipelines as array of gcli_pipeline use parse_github_checksuite);
