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

#ifndef FORGES_H
#define FORGES_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gcli/comments.h>
#include <gcli/curl.h>
#include <gcli/forks.h>
#include <gcli/issues.h>
#include <gcli/labels.h>
#include <gcli/milestones.h>
#include <gcli/pulls.h>
#include <gcli/releases.h>
#include <gcli/repos.h>
#include <gcli/review.h>
#include <gcli/sshkeys.h>
#include <gcli/status.h>

typedef struct gcli_forge_descriptor gcli_forge_descriptor;

/**
 * Struct of function pointers to perform actions in the given
 * forge. It is like a plugin system to dispatch. */
struct gcli_forge_descriptor {
	/**
	 * Submit a comment to a pull/mr or issue */
	void (*perform_submit_comment)(
		gcli_submit_comment_opts  opts,
		gcli_fetch_buffer        *out);

	/**
	 * List comments on the given issue */
	int (*get_issue_comments)(
		char const *owner,
		char const *repo,
		int issue,
		gcli_comment **out);

	/**
	 * List comments on the given PR */
	int (*get_pull_comments)(
		char const *owner,
		char const *repo,
		int pr,
		gcli_comment **out);

	/**
	 * List forks of the given repo */
	int (*get_forks)(
		char const *owner,
		char const *repo,
		int max,
		gcli_fork_list *out);

	/**
	 * Fork the given repo into the owner _in */
	void (*fork_create)(
		char const *owner,
		char const *repo,
		char const *_in);

	/**
	 * Get a list of issues on the given repo */
	int (*get_issues)(
		char const *owner,
		char const *repo,
		gcli_issue_fetch_details const *details,
		int max,
		gcli_issue_list *out);

	/**
	 * Get a summary of an issue */
	void (*get_issue_summary)(
		char const *owner,
		char const *repo,
		int issue_number,
		gcli_issue *out);

	/**
	 * Close the given issue */
	int (*issue_close)(
		char const *owner,
		char const *repo,
		int issue_number);

	/**
	 * Reopen the given issue */
	int (*issue_reopen)(
		char const *owner,
		char const *repo,
		int issue_number);

	/**
	 * Assign an issue to a user */
	int (*issue_assign)(
		char const *owner,
		char const *repo,
		int issue_number,
		char const *assignee);

	/**
	 * Add labels to issues */
	int (*issue_add_labels)(
		char const *owner,
		char const *repo,
		int issue,
		char const *const labels[],
		size_t labels_size);

	/**
	 * Removes labels from issues */
	int (*issue_remove_labels)(
		char const *owner,
		char const *repo,
		int issue,
		char const *const labels[],
		size_t labels_size);

	/**
	 * Submit an issue */
	int (*perform_submit_issue)(
		gcli_submit_issue_options opts,
		gcli_fetch_buffer *out);

	/**
	 * Bitmask of exceptions/fields that the forge doesn't support */
	enum {
		GCLI_MILESTONE_QUIRKS_EXPIRED = 0x1,
		GCLI_MILESTONE_QUIRKS_DUEDATE = 0x2,
		GCLI_MILESTONE_QUIRKS_PULLS   = 0x4,
		GCLI_MILESTONE_QUIRKS_NISSUES = 0x8,
	} const milestone_quirks;

	/**
	 * Get list of milestones */
	int (*get_milestones)(
		char const *owner,
		char const *repo,
		int max,
		gcli_milestone_list *out);

	/**
	 * Get a single milestone */
	int (*get_milestone)(
		char const *owner,
		char const *repo,
		int milestone,
		gcli_milestone *out);

	/**
	 * create a milestone */
	int (*create_milestone)(
		struct gcli_milestone_create_args const *args);

	/**
	 * delete a milestone */
	int (*delete_milestone)(
		char const *owner,
		char const *repo,
		int milestone);

	/**
	 * delete a milestone */
	int (*milestone_set_duedate)(
		char const *owner,
		char const *repo,
		int milestone,
		char const *date);

	/**
	 * Get list of issues attached to this milestone */
	int (*get_milestone_issues)(
		char const *owner,
		char const *repo,
		int milestone,
		gcli_issue_list *out);

	/** Assign an issue to a milestone */
	int (*issue_set_milestone)(
		char const *owner,
		char const *repo,
		int issue,
		int milestone);

	/**
	 * Clear the milestones of an issue */
	int (*issue_clear_milestone)(
		char const *owner,
		char const *repo,
		int issue);

	/**
	 * Get a list of PRs/MRs on the given repo */
	int (*get_prs)(
		char const *owner,
		char const *reponame,
		gcli_pull_fetch_details const *details,
		int max,
		gcli_pull_list *out);

	/**
	 * Print a diff of the changes of a PR/MR to the stream */
	void (*print_pr_diff)(
		FILE *stream,
		char const *owner,
		char const *reponame,
		int pr_number);

	/**
	 * Print a list of checks associated with the given pull.
	 *
	 * NOTE(Nico): This is a print routine here because the CI systems
	 * underlying the forge are so different that we cannot properly
	 * unify them. For Gitlab this will call into the pipelines code,
	 * for Github into the actions code. */
	int (*print_pr_checks)(
		char const *owner,
		char const *reponame,
		int pr_number);

	/**
	 * Merge the given PR/MR */
	void (*pr_merge)(
		char const *owner,
		char const *reponame,
		int pr_number,
		enum gcli_merge_flags flags);

	/**
	 * Reopen the given PR/MR */
	void (*pr_reopen)(
		char const *owner,
		char const *reponame,
		int pr_number);

	/**
	 * Close the given PR/MR */
	void (*pr_close)(
		char const *owner,
		char const *reponame,
		int pr_number);

	/**
	 * Submit PR/MR */
	void (*perform_submit_pr)(
		gcli_submit_pull_options opts);

	/**
	 * Get a list of commits in the given PR/MR */
	int (*get_pull_commits)(
		char const *owner,
		char const *repo,
		int pr_number,
		gcli_commit **out);

	/** Bitmask of unsupported fields in the pull summary for this
	 * forge */
	enum gcli_pr_summary_quirks {
		GCLI_PRS_QUIRK_ADDDEL     = 0x01,
		GCLI_PRS_QUIRK_COMMITS    = 0x02,
		GCLI_PRS_QUIRK_CHANGES    = 0x04,
		GCLI_PRS_QUIRK_MERGED     = 0x08,
		GCLI_PRS_QUIRK_DRAFT      = 0x10,
	} pull_summary_quirks;

	/**
	 * Get a summary of the given PR/MR */
	void (*get_pull)(
		char const *owner,
		char const *repo,
		int pr_number,
		gcli_pull *out);

	/**
	 * Add labels to Pull Requests */
	int (*pr_add_labels)(
		char const *owner,
		char const *repo,
		int pr,
		char const *const labels[],
		size_t labels_size);

	/**
	 * Removes labels from Pull Requests */
	int (*pr_remove_labels)(
		char const *owner,
		char const *repo,
		int pr,
		char const *const labels[],
		size_t labels_size);

	/**
	 * Assign a PR to a milestone */
	int (*pr_set_milestone)(
		char const *owner,
		char const *repo,
		int pr,
		int milestone_id);

	/**
	 * Clear a milestone on a PR */
	int (*pr_clear_milestone)(
		char const *owner,
		char const *repo,
		int pr);

	/**
	 * Get a list of releases in the given repo */
	int (*get_releases)(
		char const *owner,
		char const *repo,
		int max,
		gcli_release_list *out);

	/**
	 * Create a new release */
	int (*create_release)(
		gcli_new_release const *release);

	/**
	 * Delete the release */
	int (*delete_release)(
		char const *owner,
		char const *repo,
		char const *id);

	/**
	 * Get a list of labels that are valid in the given repository */
    int (*get_labels)(
		char const *owner,
		char const *repo,
		int max,
		gcli_label_list *out);

	/**
	 * Create the given label
	 *
	 * The ID will be filled in for you */
	int (*create_label)(
		char const *owner,
		char const *repo,
		gcli_label *label);

	/**
	 * Delete the given label */
	int (*delete_label)(
		char const *owner,
		char const *repo,
		char const *label);

	/**
	 * Get a list of repos of the given owner */
	int (*get_repos)(
		char const *owner,
		int max,
		gcli_repo_list *out);

	/**
	 * Get a list of your own repos */
	int (*get_own_repos)(
		int max,
		gcli_repo_list *out);

	/**
	 * Create the given repo */
	gcli_repo *(*repo_create)(
		gcli_repo_create_options const *options);

	/**
	 * Delete the given repo */
	void (*repo_delete)(
		char const *owner,
		char const *repo);

	/**
	 * Fetch MR/PR reviews including comments */
	size_t (*get_reviews)(
		char const *owner,
		char const *repo,
		int pr,
		gcli_pr_review **out);

	/**
	 * Status summary for the account */
	size_t (*get_notifications)(
		gcli_notification **notifications,
		int count);

	/**
	 * Mark notification with the given id as read
	 *
	 * Returns 0 on success or negative code on failure. */
	void (*notification_mark_as_read)(
		char const *id);

	/**
	 * Get an the http authentication header for use by curl */
	char *(*get_authheader)(void);

	/**
	 * Get the user account name */
	sn_sv (*get_account)(void);

	/**
	 * Get list of SSH keys */
	int (*get_sshkeys)(gcli_sshkey_list *);

	/**
	 * Add an SSH public key */
	int (*add_sshkey)(char const *title,
	                  char const *public_key_path,
	                  gcli_sshkey *out);

	/**
	 * Delete an SSH public key by its ID */
	int (*delete_sshkey)(int id);

	/**
	 * Get the error string from the API */
	char const *(*get_api_error_string)(
		gcli_fetch_buffer *);

	/**
	 * A key in the user json object sent by the API that represents
	 * the user name */
	char const *user_object_key;

	/**
	 * A key in responses by the API that represents the URL for the
	 * object being operated on */
	char const *html_url_key;
};

gcli_forge_descriptor const *gcli_forge(void);

#endif /* FORGES_H */
