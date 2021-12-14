/*
 * Copyright 2021 Nico Sonack <nsonack@outlook.com>
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

#include <ghcli/config.h>
#include <ghcli/forges.h>

#include <ghcli/github/comments.h>
#include <ghcli/github/config.h>
#include <ghcli/github/forks.h>
#include <ghcli/github/issues.h>
#include <ghcli/github/pulls.h>
#include <ghcli/github/releases.h>
#include <ghcli/github/repos.h>

#include <ghcli/gitlab/config.h>
#include <ghcli/gitlab/issues.h>

static ghcli_forge_descriptor
github_forge_descriptor =
{
    .perform_submit_comment = github_perform_submit_comment,
    .get_issue_comments     = github_get_issue_comments,
    .get_forks              = github_get_forks,
    .fork_create            = github_fork_create,
    .get_issues             = github_get_issues,
    .get_issue_summary      = github_get_issue_summary,
    .issue_close            = github_issue_close,
    .issue_reopen           = github_issue_reopen,
    .perform_submit_issue   = github_perform_submit_issue,
    .get_prs                = github_get_prs,
    .print_pr_diff          = github_print_pr_diff,
    .pr_merge               = github_pr_merge,
    .pr_reopen              = github_pr_reopen,
    .pr_close               = github_pr_close,
    .perform_submit_pr      = github_perform_submit_pr,
    .get_pull_commits       = github_get_pull_commits,
    .get_pull_summary       = github_get_pull_summary,
    .get_releases           = github_get_releases,
    .create_release         = github_create_release,
    .delete_release         = github_delete_release,
    .get_repos              = github_get_repos,
    .get_own_repos          = github_get_own_repos,
    .repo_delete            = github_repo_delete,
    .get_authheader         = github_get_authheader,
    .user_object_key        = "login",
    .html_url_key           = "html_url",
};

static ghcli_forge_descriptor
gitlab_forge_descriptor =
{
    /* .perform_submit_comment = gitlab_perform_submit_comment, */
    /* .get_issue_comments     = gitlab_get_issue_comments, */
    /* .get_forks              = gitlab_get_forks, */
    /* .fork_create            = gitlab_fork_create, */
    .get_issues             = gitlab_get_issues,
    .get_issue_summary      = gitlab_get_issue_summary,
    /* .issue_close            = gitlab_issue_close, */
    /* .issue_reopen           = gitlab_issue_reopen, */
    /* .perform_submit_issue   = gitlab_perform_submit_issue, */
    /* .get_prs                = gitlab_get_prs, */
    /* .print_pr_diff          = gitlab_print_pr_diff, */
    /* .pr_merge               = gitlab_pr_merge, */
    /* .pr_reopen              = gitlab_pr_reopen, */
    /* .pr_close               = gitlab_pr_close, */
    /* .perform_submit_pr      = gitlab_perform_submit_pr, */
    /* .get_pull_commits       = gitlab_get_pull_commits, */
    /* .get_pull_summary       = gitlab_get_pull_summary, */
    /* .get_releases           = gitlab_get_releases, */
    /* .create_release         = gitlab_create_release, */
    /* .delete_release         = gitlab_delete_release, */
    /* .get_repos              = gitlab_get_repos, */
    /* .get_own_repos          = gitlab_get_own_repos, */
    /* .repo_delete            = gitlab_repo_delete, */
    .get_authheader         = gitlab_get_authheader,
    .user_object_key        = "username",
    .html_url_key           = "web_url",
};

const ghcli_forge_descriptor *
ghcli_forge(void)
{
    switch (ghcli_config_get_forge_type()) {
    case GHCLI_FORGE_GITHUB:
        return &github_forge_descriptor;
    case GHCLI_FORGE_GITLAB:
        return &gitlab_forge_descriptor;
    default:
        sn_unimplemented;
    }
    return NULL;
}
