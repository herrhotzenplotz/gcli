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
#include "config.h"
#endif

#include <gcli/comments.h>
#include <gcli/curl.h>
#include <gcli/forks.h>
#include <gcli/issues.h>
#include <gcli/labels.h>
#include <gcli/pulls.h>
#include <gcli/releases.h>
#include <gcli/repos.h>
#include <gcli/review.h>
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
		char const    *owner,
		char const    *repo,
		int const      issue,
		gcli_comment **out);

	/**
	 * List comments on the given PR */
	int (*get_pull_comments)(
		char const           *owner,
		char const           *repo,
		int const             pr,
		gcli_comment **const  out);

	/**
	 * List forks of the given repo */
	int (*get_forks)(
		char const            *owner,
		char const            *repo,
		int const              max,
		gcli_fork_list *const  out);

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
		bool const all,
		int const max,
		gcli_issue_list *const out);

	/**
	 * Get a summary of an issue */
	void (*get_issue_summary)(
		char const        *owner,
		char const        *repo,
		int const          issue_number,
		gcli_issue *const  out);

	/**
	 * Close the given issue */
	void (*issue_close)(
		char const *owner,
		char const *repo,
		int const   issue_number);

	/**
	 * Reopen the given issue */
	void (*issue_reopen)(
		char const *owner,
		char const *repo,
		int const   issue_number);

	/**
	 * Assign an issue to a user */
	void (*issue_assign)(
		char const *owner,
		char const *repo,
		int const   issue_number,
		char const *assignee);

	/**
	 * Add labels to issues */
	void (*issue_add_labels)(
		char const        *owner,
		char const        *repo,
		int const          issue,
		char const *const  labels[],
		size_t const       labels_size);

	/**
	 * Removes labels from issues */
	void (*issue_remove_labels)(
		char const        *owner,
		char const        *repo,
		int const          issue,
		char const *const  labels[],
		size_t const       labels_size);

	/**
	 * Submit an issue */
	void (*perform_submit_issue)(
		gcli_submit_issue_options  opts,
		gcli_fetch_buffer         *out);

	/**
	 * Get a list of PRs/MRs on the given repo */
	int (*get_prs)(
		char const *owner,
		char const *reponame,
		bool const all,
		int const max,
		gcli_pull_list *const out);

	/**
	 * Print a diff of the changes of a PR/MR to the stream */
	void (*print_pr_diff)(
		FILE       *stream,
		char const *owner,
		char const *reponame,
		int const   pr_number);

	/**
	 * Print a list of checks associated with the given pull. */
	void (*print_pr_checks)(
		char const *owner,
		char const *reponame,
		int const pr_number);

	/**
	 * Merge the given PR/MR */
	void (*pr_merge)(
		char const *owner,
		char const *reponame,
		int const   pr_number,
		bool const  squash);

	/**
	 * Reopen the given PR/MR */
	void (*pr_reopen)(
		char const *owner,
		char const *reponame,
		int const   pr_number);

	/**
	 * Close the given PR/MR */
	void (*pr_close)(
		char const *owner,
		char const *reponame,
		int const   pr_number);

	/**
	 * Submit PR/MR */
	void (*perform_submit_pr)(
		gcli_submit_pull_options opts);

	/**
	 * Get a list of commits in the given PR/MR */
	int (*get_pull_commits)(
		char const          *owner,
		char const          *repo,
		int const            pr_number,
		gcli_commit **const  out);

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
	void (*get_pull_summary)(
		char const               *owner,
		char const               *repo,
		int const                 pr_number,
		gcli_pull_summary *const  out);

	/**
	 * Add labels to Pull Requests */
	void (*pr_add_labels)(
		char const        *owner,
		char const        *repo,
		int const          pr,
		char const *const  labels[],
		size_t const       labels_size);

	/**
	 * Removes labels from Pull Requests */
	void (*pr_remove_labels)(
		char const        *owner,
		char const        *repo,
		int const          pr,
		char const *const  labels[],
		size_t const       labels_size);

	/**
	 * Get a list of releases in the given repo */
	int (*get_releases)(
		char const           *owner,
		char const           *repo,
		int const             max,
		gcli_release **const  out);

	/**
	 * Create a new release */
	void (*create_release)(
		gcli_new_release const *release);

	/**
	 * Delete the release */
	void (*delete_release)(
		char const *owner,
		char const *repo,
		char const *id);

	/**
	 * Get a list of labels that are valid in the given repository */
    int (*get_labels)(
		char const             *owner,
		char const             *repo,
		int const               max,
		gcli_label_list *const  out);

	/**
	 * Create the given label
	 *
	 * The ID will be filled in for you */
	void (*create_label)(
		char const        *owner,
		char const        *repo,
		gcli_label *const  label);

	/**
	 * Delete the given label */
	void (*delete_label)(
		char const *owner,
		char const *repo,
		char const *label);

	/**
	 * Get a list of repos of the given owner */
	int (*get_repos)(
		char const *owner,
		int const max,
		gcli_repo_list *const out);

	/**
	 * Get a list of your own repos */
	int (*get_own_repos)(
		int max,
		gcli_repo_list *const out);

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
		char const      *owner,
		char const      *repo,
		int              pr,
		gcli_pr_review **out);

	/**
	 * Status summary for the account */
	size_t (*get_notifications)(gcli_notification **notifications, int count);

	/**
	 * Mark notification with the given id as read
	 *
	 * Returns 0 on success or negative code on failure. */
	void (*notification_mark_as_read)(char const *id);

	/**
	 * Get an the http authentication header for use by curl */
	char *(*get_authheader)(void);

	/**
	 * Get the user account name */
	sn_sv (*get_account)(void);

	/**
	 * Get the error string from the API */
	char const *(*get_api_error_string)(gcli_fetch_buffer *);

	/**
	 * A key in the user json object sent by the API that represents
	 * the user name */
	char const *user_object_key;

	/**
	 * A key in responses by the API that represents the URL for the
	 * object being operated on */
	char const *html_url_key;
};

const gcli_forge_descriptor *gcli_forge(void);

#endif /* FORGES_H */
