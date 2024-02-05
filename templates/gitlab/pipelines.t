include "gcli/gitlab/pipelines.h";

parser gitlab_pipeline is
object of struct gitlab_pipeline with
	("status"     => status as string,
	 "created_at" => created_at as string,
	 "updated_at" => updated_at as string,
	 "ref"        => ref as string,
	 "sha"        => sha as string,
	 "source"     => source as string,
	 "id"         => id as id);

parser gitlab_pipelines is
array of struct gitlab_pipeline use parse_gitlab_pipeline;

parser gitlab_job_runner is
object of struct gitlab_job with
	("name"        => runner_name as string,
	 "description" => runner_description as string);

parser gitlab_job is
object of struct gitlab_job with
	("status"      => status as string,
	 "stage"       => stage as string,
	 "name"        => name as string,
	 "ref"         => ref as string,
	 "created_at"  => created_at as string,
	 "started_at"  => started_at as string,
	 "finished_at" => finished_at as string,
	 "runner"      => use parse_gitlab_job_runner,
	 "duration"    => duration as double,
	 "id"          => id as id,
	 "coverage"    => coverage as double);

parser gitlab_jobs is
array of struct gitlab_job use parse_gitlab_job;
