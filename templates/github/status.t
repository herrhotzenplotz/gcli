include "gcli/github/status.h";
include "gcli/json_util.h";

parser github_notification_subject is
object of gcli_notification with
	   ("title" => title as string,
		"type" => type as string);

parser github_notification_repository is
object of gcli_notification with
	   ("full_name" => repository as string);

parser github_notification is
object of gcli_notification with
	   ("updated_at" => date as string,
		"id" => id as string,
		"reason" => reason as string,
		"subject" => use parse_github_notification_subject,
		"repository" => use parse_github_notification_repository);

parser github_notifications is
array of gcli_notification use parse_github_notification;