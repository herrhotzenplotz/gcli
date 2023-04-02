/*
 * Copyright 2021, 2022 Nico Sonack <nsonack@herrhotzenplotz.de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>

#include <gcli/config.h>
#include <gcli/forges.h>

#include <gcli/github/api.h>
#include <gcli/github/comments.h>
#include <gcli/github/config.h>
#include <gcli/github/forks.h>
#include <gcli/github/issues.h>
#include <gcli/github/labels.h>
#include <gcli/github/milestones.h>
#include <gcli/github/pulls.h>
#include <gcli/github/releases.h>
#include <gcli/github/repos.h>
#include <gcli/github/review.h>
#include <gcli/github/status.h>

#include <gcli/gitlab/api.h>
#include <gcli/gitlab/comments.h>
#include <gcli/gitlab/config.h>
#include <gcli/gitlab/forks.h>
#include <gcli/gitlab/issues.h>
#include <gcli/gitlab/labels.h>
#include <gcli/gitlab/merge_requests.h>
#include <gcli/gitlab/milestones.h>
#include <gcli/gitlab/pipelines.h>
#include <gcli/gitlab/releases.h>
#include <gcli/gitlab/repos.h>
#include <gcli/gitlab/review.h>
#include <gcli/gitlab/status.h>

#include <gcli/gitea/comments.h>
#include <gcli/gitea/config.h>
#include <gcli/gitea/forks.h>
#include <gcli/gitea/issues.h>
#include <gcli/gitea/labels.h>
#include <gcli/gitea/milestones.h>
#include <gcli/gitea/pulls.h>
#include <gcli/gitea/releases.h>
#include <gcli/gitea/repos.h>

static gcli_forge_descriptor const
github_forge_descriptor =
{
	.perform_submit_comment    = github_perform_submit_comment,
	.get_issue_comments        = github_get_comments,
	.get_pull_comments         = github_get_comments,
	.get_forks                 = github_get_forks,
	.fork_create               = github_fork_create,
	.get_issues                = github_get_issues,
	.get_issue_summary         = github_get_issue_summary,
	.issue_close               = github_issue_close,
	.issue_reopen              = github_issue_reopen,
	.issue_assign              = github_issue_assign,
	.issue_add_labels          = github_issue_add_labels,
	.issue_remove_labels       = github_issue_remove_labels,
	.perform_submit_issue      = github_perform_submit_issue,
	.issue_set_milestone       = github_issue_set_milestone,
	.get_milestones            = github_get_milestones,
	.get_milestone             = github_get_milestone,
	.create_milestone          = github_create_milestone,
	.delete_milestone          = github_delete_milestone,
	.milestone_set_duedate     = github_milestone_set_duedate,
	.get_milestone_issues      = github_milestone_get_issues,
	.get_prs                   = github_get_pulls,
	.print_pr_diff             = github_print_pull_diff,
	.print_pr_checks           = github_pull_checks,
	.pr_merge                  = github_pull_merge,
	.pr_reopen                 = github_pull_reopen,
	.pr_close                  = github_pull_close,

	/* HACK: Here we can use the same functions as with issues because
	 * PRs are the same as issues on Github and the functions have the
	 * same types/arguments */
	.pr_add_labels             = github_issue_add_labels,
	.pr_remove_labels          = github_issue_remove_labels,

	.perform_submit_pr         = github_perform_submit_pull,
	.get_pull_commits          = github_get_pull_commits,
	.get_pull                  = github_get_pull,
	.get_releases              = github_get_releases,
	.create_release            = github_create_release,
	.delete_release            = github_delete_release,
	.get_labels                = github_get_labels,
	.create_label              = github_create_label,
	.delete_label              = github_delete_label,
	.get_repos                 = github_get_repos,
	.get_own_repos             = github_get_own_repos,
	.get_reviews               = github_review_get_reviews,
	.repo_create               = github_repo_create,
	.repo_delete               = github_repo_delete,
	.get_notifications         = github_get_notifications,
	.notification_mark_as_read = github_notification_mark_as_read,
	.get_authheader            = github_get_authheader,
	.get_account               = github_get_account,
	.get_api_error_string      = github_api_error_string,
	.user_object_key           = "login",
	.html_url_key              = "html_url",
	.pull_summary_quirks       = 0,
	.milestone_quirks          = GCLI_MILESTONE_QUIRKS_EXPIRED
	                           | GCLI_MILESTONE_QUIRKS_DUEDATE
	                           | GCLI_MILESTONE_QUIRKS_PULLS,
};

static gcli_forge_descriptor const
gitlab_forge_descriptor =
{
	.perform_submit_comment    = gitlab_perform_submit_comment,
	.get_issue_comments        = gitlab_get_issue_comments,
	.get_pull_comments         = gitlab_get_mr_comments,
	.get_forks                 = gitlab_get_forks,
	.fork_create               = gitlab_fork_create,
	.get_issues                = gitlab_get_issues,
	.get_issue_summary         = gitlab_get_issue_summary,
	.issue_close               = gitlab_issue_close,
	.issue_reopen              = gitlab_issue_reopen,
	.issue_assign              = gitlab_issue_assign,
	.issue_add_labels          = gitlab_issue_add_labels,
	.issue_remove_labels       = gitlab_issue_remove_labels,
	.perform_submit_issue      = gitlab_perform_submit_issue,
	.get_milestones            = gitlab_get_milestones,
	.get_milestone             = gitlab_get_milestone,
	.create_milestone          = gitlab_create_milestone,
	.delete_milestone          = gitlab_delete_milestone,
	.get_milestone_issues      = gitlab_milestone_get_issues,
	.issue_set_milestone       = gitlab_issue_set_milestone,
	.get_prs                   = gitlab_get_mrs,
	.print_pr_diff             = gitlab_print_pr_diff,
	.print_pr_checks           = gitlab_mr_pipelines,
	.pr_merge                  = gitlab_mr_merge,
	.pr_reopen                 = gitlab_mr_reopen,
	.pr_close                  = gitlab_mr_close,
	.perform_submit_pr         = gitlab_perform_submit_mr,
	.get_pull_commits          = gitlab_get_pull_commits,
	.get_pull                  = gitlab_get_pull,
	.pr_add_labels             = gitlab_mr_add_labels,
	.pr_remove_labels          = gitlab_mr_remove_labels,
	.get_releases              = gitlab_get_releases,
	.create_release            = gitlab_create_release,
	.delete_release            = gitlab_delete_release,
	.get_labels                = gitlab_get_labels,
	.create_label              = gitlab_create_label,
	.delete_label              = gitlab_delete_label,
	.get_repos                 = gitlab_get_repos,
	.get_own_repos             = gitlab_get_own_repos,
	.get_reviews               = gitlab_review_get_reviews,
	.repo_create               = gitlab_repo_create,
	.repo_delete               = gitlab_repo_delete,
	.get_notifications         = gitlab_get_notifications,
	.notification_mark_as_read = gitlab_notification_mark_as_read,
	.get_authheader            = gitlab_get_authheader,
	.get_account               = gitlab_get_account,
	.get_api_error_string      = gitlab_api_error_string,
	.user_object_key           = "username",
	.html_url_key              = "web_url",
	.pull_summary_quirks       = GCLI_PRS_QUIRK_ADDDEL
	                           | GCLI_PRS_QUIRK_COMMITS
	                           | GCLI_PRS_QUIRK_CHANGES
	                           | GCLI_PRS_QUIRK_MERGED,
	.milestone_quirks          = GCLI_MILESTONE_QUIRKS_NISSUES,
};

static gcli_forge_descriptor const
gitea_forge_descriptor =
{
	.get_issues                = gitea_get_issues,
	.get_issue_summary         = gitea_get_issue_summary,
	.perform_submit_issue      = gitea_submit_issue,
	.issue_close               = gitea_issue_close,
	.issue_reopen              = gitea_issue_reopen,
	.issue_assign              = gitea_issue_assign,
	.issue_set_milestone       = gitea_issue_set_milestone,
	.get_issue_comments        = gitea_get_comments,
	.get_milestones            = gitea_get_milestones,
	.get_milestone             = gitea_get_milestone,
	.create_milestone          = gitea_create_milestone,
	.delete_milestone          = gitea_delete_milestone,
	.perform_submit_comment    = gitea_perform_submit_comment,
	.issue_add_labels          = gitea_issue_add_labels,
	.issue_remove_labels       = gitea_issue_remove_labels,
	.get_labels                = gitea_get_labels,
	.create_label              = gitea_create_label,
	.delete_label              = gitea_delete_label,
	.get_prs                   = gitea_get_pulls,
	.print_pr_checks           = gitea_pull_checks, /* stub */
	.pr_merge                  = gitea_pull_merge,
	.pr_reopen                 = gitea_pull_reopen,
	.pr_close                  = gitea_pull_close,
	.get_pull_comments         = gitea_get_comments,
	.get_pull                  = gitea_get_pull,
	.get_pull_commits          = gitea_get_pull_commits,
	.get_releases              = gitea_get_releases,
	.create_release            = gitea_create_release,
	.delete_release            = gitea_delete_release,
	.perform_submit_pr         = gitea_pull_submit,
	.print_pr_diff             = gitea_print_pr_diff,
	.get_forks                 = gitea_get_forks,

	/* Same procedure as with Github (see comment up there) */
	.pr_add_labels             = gitea_issue_add_labels,
	.pr_remove_labels          = gitea_issue_remove_labels,
	.get_repos                 = gitea_get_repos,
	.get_own_repos             = gitea_get_own_repos,
	.repo_create               = gitea_repo_create,
	.repo_delete               = gitea_repo_delete,

	.get_authheader            = gitea_get_authheader,
	.get_account               = gitea_get_account,
	.get_api_error_string      = github_api_error_string,    /* hack! */
	.user_object_key           = "username",
	.html_url_key              = "web_url",
	.pull_summary_quirks       = GCLI_PRS_QUIRK_COMMITS
	                           | GCLI_PRS_QUIRK_ADDDEL
	                           | GCLI_PRS_QUIRK_DRAFT
	                           | GCLI_PRS_QUIRK_CHANGES,
	.milestone_quirks          = GCLI_MILESTONE_QUIRKS_EXPIRED
	                           | GCLI_MILESTONE_QUIRKS_DUEDATE
	                           | GCLI_MILESTONE_QUIRKS_PULLS,
};

gcli_forge_descriptor const *
gcli_forge(void)
{
	switch (gcli_config_get_forge_type()) {
	case GCLI_FORGE_GITHUB:
		return &github_forge_descriptor;
	case GCLI_FORGE_GITLAB:
		return &gitlab_forge_descriptor;
	case GCLI_FORGE_GITEA:
		return &gitea_forge_descriptor;
	default:
		errx(1,
		     "error: cannot determine forge type. try forcing an account "
		     "with -a, specifying -t or create a .gcli file.");
	}
	return NULL;
}
