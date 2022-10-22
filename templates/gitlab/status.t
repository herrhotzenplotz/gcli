include "gcli/gitlab/status.h";

parser gitlab_project is
object of gcli_notification with
       ("path_with_namespace" => repository as string);

parser gitlab_todo is
object of gcli_notification with
       ("updated_at" => date as string,
        "action_name" => reason as string,
        "id" => id as int_to_string,
        "body" => title as string,
        "target_type" => type as string,
        "project" => use parse_gitlab_project);

parser gitlab_todos is
array of gcli_notification use parse_gitlab_todo;
