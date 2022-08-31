/*
 * Copyright 2021 Nico Sonack <nsonack@herrhotzenplotz.de>
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
		const char     *owner,
		const char     *repo,
		int             issue,
		gcli_comment **out);

	/**
	 * List comments on the given PR */
	int (*get_pull_comments)(
		const char     *owner,
		const char     *repo,
		int             pr,
		gcli_comment **out);

	/**
	 * List forks of the given repo */
	int (*get_forks)(
		const char  *owner,
		const char  *repo,
		int          max,
		gcli_fork **out);

	/**
	 * Fork the given repo into the owner _in */
	void (*fork_create)(
		const char *owner,
		const char *repo,
		const char *_in);

	/**
	 * Get a list of issues on the given repo */
	int (*get_issues)(
		const char   *owner,
		const char   *repo,
		bool          all,
		int           max,
		gcli_issue **out);

	/**
	 * Get a summary of an issue */
	void (*get_issue_summary)(
		const char          *owner,
		const char          *repo,
		int                  issue_number,
		gcli_issue			*out);

	/**
	 * Close the given issue */
	void (*issue_close)(
		const char *owner,
		const char *repo,
		int         issue_number);

	/**
	 * Reopen the given issue */
	void (*issue_reopen)(
		const char *owner,
		const char *repo,
		int         issue_number);

	/**
	 * Assign an issue to a user */
	void (*issue_assign)(
		const char *owner,
		const char *repo,
		int         issue_number,
		const char *assignee);

	/**
	 * Add labels to issues */
	void (*issue_add_labels)(
		const char *owner,
		const char *repo,
		int         issue,
		const char *labels[],
		size_t      labels_size);

	/**
	 * Removes labels from issues */
	void (*issue_remove_labels)(
		const char *owner,
		const char *repo,
		int         issue,
		const char *labels[],
		size_t      labels_size);

	/**
	 * Submit an issue */
	void (*perform_submit_issue)(
		gcli_submit_issue_options  opts,
		gcli_fetch_buffer         *out);

	/**
	 * Get a list of PRs/MRs on the given repo */
	int (*get_prs)(
		const char  *owner,
		const char  *reponame,
		bool         all,
		int          max,
		gcli_pull **out);

	/**
	 * Print a diff of the changes of a PR/MR to the stream */
	void (*print_pr_diff)(
		FILE       *stream,
		const char *owner,
		const char *reponame,
		int         pr_number);

	/**
	 * Merge the given PR/MR */
	void (*pr_merge)(
		const char *owner,
		const char *reponame,
		int         pr_number,
		bool        squash);

	/**
	 * Reopen the given PR/MR */
	void (*pr_reopen)(
		const char *owner,
		const char *reponame,
		int         pr_number);

	/**
	 * Close the given PR/MR */
	void (*pr_close)(
		const char *owner,
		const char *reponame,
		int         pr_number);

	/**
	 * Submit PR/MR */
	void (*perform_submit_pr)(
		gcli_submit_pull_options  opts);

	/**
	 * Get a list of commits in the given PR/MR */
	int (*get_pull_commits)(
		const char    *owner,
		const char    *repo,
		int            pr_number,
		gcli_commit **out);

	/**
	 * Get a summary of the given PR/MR */
	void (*get_pull_summary)(
		const char         *owner,
		const char         *repo,
		int                 pr_number,
		gcli_pull_summary *out);

	/**
	 * Add labels to Pull Requests */
	void (*pr_add_labels)(
		const char *owner,
		const char *repo,
		int         pr,
		const char *labels[],
		size_t      labels_size);

	/**
	 * Removes labels from Pull Requests */
	void (*pr_remove_labels)(
		const char *owner,
		const char *repo,
		int         pr,
		const char *labels[],
		size_t      labels_size);

	/**
	 * Get a list of releases in the given repo */
	int (*get_releases)(
		const char     *owner,
		const char     *repo,
		int             max,
		gcli_release **out);

	/**
	 * Create a new release */
	void (*create_release)(
		const gcli_new_release *release);

	/**
	 * Delete the release */
	void (*delete_release)(
		const char *owner,
		const char *repo,
		const char *id);

	/**
	 * Get a list of labels that are valid in the given repository */
	size_t (*get_labels)(
		const char   *owner,
		const char   *repo,
		int           max,
		gcli_label **out);

	/**
	 * Create the given label
	 *
	 * The ID will be filled in for you */
	void (*create_label)(
		const char  *owner,
		const char  *repo,
		gcli_label *label);

	/**
	 * Delete the given label */
	void (*delete_label)(
		const char *owner,
		const char *repo,
		const char *label);

	/**
	 * Get a list of repos of the given owner */
	int (*get_repos)(
		const char  *owner,
		int          max,
		gcli_repo **out);

	/**
	 * Get a list of your own repos */
	int (*get_own_repos)(
		int          max,
		gcli_repo **out);

	/**
	 * Create the given repo */
	gcli_repo *(*repo_create)(
		const gcli_repo_create_options *options);

	/**
	 * Delete the given repo */
	void (*repo_delete)(
		const char *owner,
		const char *repo);

	/**
	 * Fetch MR/PR reviews including comments */
	size_t (*get_reviews)(
		const char       *owner,
		const char       *repo,
		int               pr,
		gcli_pr_review **out);

	/**
	 * Status summary for the account */
	size_t (*get_notifications)(gcli_notification **notifications, int count);

	/**
	 * Mark notification with the given id as read
	 *
	 * Returns 0 on success or negative code on failure. */
	void (*notification_mark_as_read)(const char *id);

	/**
	 * Get an the http authentication header for use by curl */
	char *(*get_authheader)(void);

	/**
	 * Get the user account name */
	sn_sv (*get_account)(void);

	/**
	 * Get the error string from the API */
	const char *(*get_api_error_string)(gcli_fetch_buffer *);

	/**
	 * A key in the user json object sent by the API that represents
	 * the user name */
	const char *user_object_key;

	/**
	 * A key in responses by the API that represents the URL for the
	 * object being operated on */
	const char *html_url_key;
};

const gcli_forge_descriptor *gcli_forge(void);

#endif /* FORGES_H */
