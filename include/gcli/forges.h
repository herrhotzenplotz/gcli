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

/* Hopefully temporary hack */
typedef int (*gcli_get_pull_checks_cb)(
	gcli_ctx *, char const *, char const *, int,
	gcli_pull_checks_list *);

/**
 * Struct of function pointers to perform actions in the given
 * forge. It is like a plugin system to dispatch. */
struct gcli_forge_descriptor {
	/**
	 * Submit a comment to a pull/mr or issue */
	int (*perform_submit_comment)(
		gcli_ctx *ctx,
		gcli_submit_comment_opts  opts,
		gcli_fetch_buffer        *out);

	/**
	 * List comments on the given issue */
	int (*get_issue_comments)(
		gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		int issue,
		gcli_comment_list *out);

	/**
	 * List comments on the given PR */
	int (*get_pull_comments)(
		gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		int pr,
		gcli_comment_list *out);

	/**
	 * List forks of the given repo */
	int (*get_forks)(
		gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		int max,
		gcli_fork_list *out);

	/**
	 * Fork the given repo into the owner _in */
	int (*fork_create)(
		gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		char const *_in);

	/**
	 * Get a list of issues on the given repo */
	int (*get_issues)(
		gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		gcli_issue_fetch_details const *details,
		int max,
		gcli_issue_list *out);

	/**
	 * Get a summary of an issue */
	int (*get_issue_summary)(
		gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		int issue_number,
		gcli_issue *out);

	/**
	 * Close the given issue */
	int (*issue_close)(
		gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		int issue_number);

	/**
	 * Reopen the given issue */
	int (*issue_reopen)(
		gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		int issue_number);

	/**
	 * Assign an issue to a user */
	int (*issue_assign)(
		gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		int issue_number,
		char const *assignee);

	/**
	 * Add labels to issues */
	int (*issue_add_labels)(
		gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		int issue,
		char const *const labels[],
		size_t labels_size);

	/**
	 * Removes labels from issues */
	int (*issue_remove_labels)(
		gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		int issue,
		char const *const labels[],
		size_t labels_size);

	/**
	 * Submit an issue */
	int (*perform_submit_issue)(
		gcli_ctx *ctx,
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
		gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		int max,
		gcli_milestone_list *out);

	/**
	 * Get a single milestone */
	int (*get_milestone)(
		gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		int milestone,
		gcli_milestone *out);

	/**
	 * create a milestone */
	int (*create_milestone)(
		gcli_ctx *ctx,
		struct gcli_milestone_create_args const *args);

	/**
	 * delete a milestone */
	int (*delete_milestone)(
		gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		int milestone);

	/**
	 * delete a milestone */
	int (*milestone_set_duedate)(
		gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		int milestone,
		char const *date);

	/**
	 * Get list of issues attached to this milestone */
	int (*get_milestone_issues)(
		gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		int milestone,
		gcli_issue_list *out);

	/** Assign an issue to a milestone */
	int (*issue_set_milestone)(
		gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		int issue,
		int milestone);

	/**
	 * Clear the milestones of an issue */
	int (*issue_clear_milestone)(
		gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		int issue);

	/**
	 * Get a list of PRs/MRs on the given repo */
	int (*get_pulls)(
		gcli_ctx *ctx,
		char const *owner,
		char const *reponame,
		gcli_pull_fetch_details const *details,
		int max,
		gcli_pull_list *out);

	/**
	 * Print a diff of the changes of a PR/MR to the stream */
	int (*print_pull_diff)(
		gcli_ctx *ctx,
		FILE *stream,
		char const *owner,
		char const *reponame,
		int pr_number);

	/**
	 * Return a list of checks associated with the given pull.
	 *
	 * The type of the returned list depends on the forge type. See
	 * the definition of gcli_pull_checks_list. */
	gcli_get_pull_checks_cb get_pull_checks;

	/**
	 * Merge the given PR/MR */
	int (*pull_merge)(
		gcli_ctx *ctx,
		char const *owner,
		char const *reponame,
		int pr_number,
		enum gcli_merge_flags flags);

	/**
	 * Reopen the given PR/MR */
	int (*pull_reopen)(
		gcli_ctx *ctx,
		char const *owner,
		char const *reponame,
		int pr_number);

	/**
	 * Close the given PR/MR */
	int (*pull_close)(
		gcli_ctx *ctx,
		char const *owner,
		char const *reponame,
		int pr_number);

	/**
	 * Submit PR/MR */
	int (*perform_submit_pull)(
		gcli_ctx *ctx,
		gcli_submit_pull_options opts);

	/**
	 * Get a list of commits in the given PR/MR */
	int (*get_pull_commits)(
		gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		int pr_number,
		gcli_commit_list *out);

	/** Bitmask of unsupported fields in the pull summary for this
	 * forge */
	enum gcli_pull_summary_quirks {
		GCLI_PRS_QUIRK_ADDDEL     = 0x01,
		GCLI_PRS_QUIRK_COMMITS    = 0x02,
		GCLI_PRS_QUIRK_CHANGES    = 0x04,
		GCLI_PRS_QUIRK_MERGED     = 0x08,
		GCLI_PRS_QUIRK_DRAFT      = 0x10,
	} pull_summary_quirks;

	/**
	 * Get a summary of the given PR/MR */
	int (*get_pull)(
		gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		int pr_number,
		gcli_pull *out);

	/**
	 * Add labels to Pull Requests */
	int (*pull_add_labels)(
		gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		int pr,
		char const *const labels[],
		size_t labels_size);

	/**
	 * Removes labels from Pull Requests */
	int (*pull_remove_labels)(
		gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		int pr,
		char const *const labels[],
		size_t labels_size);

	/**
	 * Assign a PR to a milestone */
	int (*pull_set_milestone)(
		gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		int pull,
		int milestone_id);

	/**
	 * Clear a milestone on a PR */
	int (*pull_clear_milestone)(
		gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		int pull);

	/**
	 * Get a list of releases in the given repo */
	int (*get_releases)(
		gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		int max,
		gcli_release_list *out);

	/**
	 * Create a new release */
	int (*create_release)(
		gcli_ctx *ctx,
		gcli_new_release const *release);

	/**
	 * Delete the release */
	int (*delete_release)(
		gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		char const *id);

	/**
	 * Get a list of labels that are valid in the given repository */
    int (*get_labels)(
	    gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		int max,
		gcli_label_list *out);

	/**
	 * Create the given label
	 *
	 * The ID will be filled in for you */
	int (*create_label)(
	    gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		gcli_label *label);

	/**
	 * Delete the given label */
	int (*delete_label)(
	    gcli_ctx *ctx,
		char const *owner,
		char const *repo,
		char const *label);

	/**
	 * Get a list of repos of the given owner */
	int (*get_repos)(
		gcli_ctx *ctx,
		char const *owner,
		int max,
		gcli_repo_list *out);

	/**
	 * Create the given repo */
	int (*repo_create)(
		gcli_ctx *ctx,
		gcli_repo_create_options const *options,
		gcli_repo *out);

	/**
	 * Delete the given repo */
	int (*repo_delete)(
		gcli_ctx *ctx,
		char const *owner,
		char const *repo);

	/**
	 * Fetch MR/PR reviews including comments */
    int (*get_reviews)(
	    gcli_ctx *ctx, char const *owner, char const *repo,
		int pr, gcli_pr_review_list *out);

	/**
	 * Status summary for the account */
    int (*get_notifications)(
	    gcli_ctx *ctx,
	    int max,
		gcli_notification_list *notifications);

	/**
	 * Mark notification with the given id as read
	 *
	 * Returns 0 on success or negative code on failure. */
	int (*notification_mark_as_read)(
		gcli_ctx *ctx,
		char const *id);

	/**
	 * Get an the http authentication header for use by curl */
	char *(*make_authheader)(gcli_ctx *ctx, char const *token);

	/**
	 * Get list of SSH keys */
	int (*get_sshkeys)(gcli_ctx *ctx, gcli_sshkey_list *);

	/**
	 * Add an SSH public key */
	int (*add_sshkey)(
		gcli_ctx *ctx,
		char const *title,
		char const *public_key_path,
		gcli_sshkey *out);

	/**
	 * Delete an SSH public key by its ID */
	int (*delete_sshkey)(gcli_ctx *ctx, int id);

	/**
	 * Get the error string from the API */
	char const *(*get_api_error_string)(
		gcli_ctx *ctx,
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

gcli_forge_descriptor const *gcli_forge(gcli_ctx *ctx);

#endif /* FORGES_H */
