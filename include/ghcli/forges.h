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

#ifndef FORGES_H
#define FORGES_H

#include <ghcli/comments.h>
#include <ghcli/curl.h>
#include <ghcli/forks.h>
#include <ghcli/issues.h>
#include <ghcli/pulls.h>
#include <ghcli/releases.h>
#include <ghcli/repos.h>
#include <ghcli/review.h>

typedef struct ghcli_forge_descriptor ghcli_forge_descriptor;

/**
 * Struct of function pointers to perform actions in the given
 * forge. It is like a plugin system to dispatch. */
struct ghcli_forge_descriptor {
    /**
     * Submit a comment to a pull/mr or issue */
    void (*perform_submit_comment)(
        ghcli_submit_comment_opts  opts,
        ghcli_fetch_buffer        *out);

    /**
     * List comments on the given issue */
    int (*get_issue_comments)(
        const char     *owner,
        const char     *repo,
        int             issue,
        ghcli_comment **out);

    /**
     * List comments on the given PR */
    int (*get_pull_comments)(
        const char     *owner,
        const char     *repo,
        int             pr,
        ghcli_comment **out);

    /**
     * List forks of the given repo */
    int (*get_forks)(
        const char  *owner,
        const char  *repo,
        int          max,
        ghcli_fork **out);

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
        ghcli_issue **out);

    /**
     * Get a summary of an issue */
    void (*get_issue_summary)(
        const char          *owner,
        const char          *repo,
        int                  issue_number,
        ghcli_issue_details *out);

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
     * Submit an issue */
    void (*perform_submit_issue)(
        ghcli_submit_issue_options  opts,
        ghcli_fetch_buffer         *out);

    /**
     * Get a list of PRs/MRs on the given repo */
    int (*get_prs)(
        const char  *owner,
        const char  *reponame,
        bool         all,
        int          max,
        ghcli_pull **out);

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
        FILE       *out,
        const char *owner,
        const char *reponame,
        int         pr_number);

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
        ghcli_submit_pull_options  opts,
        ghcli_fetch_buffer        *out);

    /**
     * Get a list of commits in the given PR/MR */
    int (*get_pull_commits)(
        const char    *owner,
        const char    *repo,
        int            pr_number,
        ghcli_commit **out);

    /**
     * Get a summary of the given PR/MR */
    void (*get_pull_summary)(
        const char         *owner,
        const char         *repo,
        int                 pr_number,
        ghcli_pull_summary *out);

    /**
     * Get a list of releases in the given repo */
    int (*get_releases)(
        const char     *owner,
        const char     *repo,
        int             max,
        ghcli_release **out);

    /**
     * Create a new release */
    void (*create_release)(
        const ghcli_new_release *release);

    /**
     * Delete the release */
    void (*delete_release)(
        const char *owner,
        const char *repo,
        const char *id);

    /**
     * Get a list of repos of the given owner */
    int (*get_repos)(
        const char  *owner,
        int          max,
        ghcli_repo **out);

    /**
     * Get a list of your own repos */
    int (*get_own_repos)(
        int          max,
        ghcli_repo **out);

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
        ghcli_pr_review **out);

    /**
     * Status summary for the account */
    void (*status)(void);

    /**
     * Get an the http authentication header for use by curl */
    char *(*get_authheader)(void);

    /**
     * Get the user account name */
    sn_sv (*get_account)(void);

    /**
     * Get the error string from the API */
    const char *(*get_api_error_string)(ghcli_fetch_buffer *);

    /**
     * A key in the user json object sent by the API that represents
     * the user name */
    const char *user_object_key;

    /**
     * A key in responses by the API that represents the URL for the
     * object being operated on */
    const char *html_url_key;
};

const ghcli_forge_descriptor *ghcli_forge(void);

#endif /* FORGES_H */
