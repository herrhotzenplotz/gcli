include "gcli/gitlab/review.h";

parser gitlab_review_note is
object of gcli_pr_review with
       ("created_at" => date as string,
        "body" => body as string,
        "author" => author as user,
        "id" => id as int_to_string);

parser gitlab_reviews is
array of gcli_pr_review use parse_gitlab_review_note;