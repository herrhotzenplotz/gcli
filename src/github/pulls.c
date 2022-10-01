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

#include <gcli/config.h>
#include <gcli/curl.h>
#include <gcli/gitconfig.h>
#include <gcli/github/config.h>
#include <gcli/github/issues.h>
#include <gcli/github/pulls.h>
#include <gcli/json_util.h>

#include <pdjson/pdjson.h>

#include <templates/github/pulls.h>

int
github_get_prs(
    const char  *owner,
    const char  *repo,
    bool         all,
    int          max,
    gcli_pull  **out)
{
    size_t             count       = 0;
    json_stream        stream      = {0};
    gcli_fetch_buffer  json_buffer = {0};
    char              *url         = NULL;
    char              *next_url    = NULL;
    char              *e_owner     = NULL;
    char              *e_repo      = NULL;

    e_owner = gcli_urlencode(owner);
    e_repo  = gcli_urlencode(repo);

    url = sn_asprintf(
        "%s/repos/%s/%s/pulls?state=%s",
        gcli_get_apibase(),
        e_owner, e_repo, all ? "all" : "open");

    do {
        gcli_fetch(url, &next_url, &json_buffer);

        json_open_buffer(&stream, json_buffer.data, json_buffer.length);

        parse_github_pulls(&stream, out, &count);

        free(json_buffer.data);
        free(url);
        json_close(&stream);
    } while ((url = next_url) && (max == -1 || count < max));

    free(url);
    free(e_owner);
    free(e_repo);

    return (int)count;
}

void
github_print_pr_diff(
    FILE       *stream,
    const char *owner,
    const char *repo,
    int         pr_number)
{
    char *url     = NULL;
    char *e_owner = NULL;
    char *e_repo  = NULL;

    e_owner = gcli_urlencode(owner);
    e_repo  = gcli_urlencode(repo);

    url = sn_asprintf(
        "%s/repos/%s/%s/pulls/%d",
        gcli_get_apibase(),
        e_owner, e_repo, pr_number);
    gcli_curl(stream, url, "Accept: application/vnd.github.v3.diff");

    free(e_owner);
    free(e_repo);
    free(url);
}

void
github_pr_merge(
    const char *owner,
    const char *repo,
    int         pr_number,
    bool        squash)
{
    json_stream        stream      = {0};
    gcli_fetch_buffer  json_buffer = {0};
    char              *url         = NULL;
    char              *e_owner     = NULL;
    char              *e_repo      = NULL;
    const char        *data        = "{}";
    char              *message;

    e_owner = gcli_urlencode(owner);
    e_repo  = gcli_urlencode(repo);

    url = sn_asprintf(
        "%s/repos/%s/%s/pulls/%d/merge?merge_method=%s",
        gcli_get_apibase(),
        e_owner, e_repo, pr_number,
        squash ? "squash" : "merge");

    gcli_fetch_with_method("PUT", url, data, NULL, &json_buffer);
    json_open_buffer(&stream, json_buffer.data, json_buffer.length);

    parse_github_pr_merge_message(&stream, &message);

    puts(message);

    json_close(&stream);
    free(message);
    free(json_buffer.data);
    free(url);
    free(e_owner);
    free(e_repo);
}

void
github_pr_close(const char *owner, const char *repo, int pr_number)
{
    gcli_fetch_buffer  json_buffer = {0};
    char              *url         = NULL;
    char              *e_owner     = NULL;
    char              *e_repo      = NULL;
    char              *data        = NULL;

    e_owner = gcli_urlencode(owner);
    e_repo  = gcli_urlencode(repo);

    url  = sn_asprintf(
        "%s/repos/%s/%s/pulls/%d",
        gcli_get_apibase(),
        e_owner, e_repo, pr_number);
    data = sn_asprintf("{ \"state\": \"closed\"}");

    gcli_fetch_with_method("PATCH", url, data, NULL, &json_buffer);

    free(json_buffer.data);
    free(url);
    free(e_repo);
    free(e_owner);
    free(data);
}

void
github_pr_reopen(const char *owner, const char *repo, int pr_number)
{
    gcli_fetch_buffer  json_buffer = {0};
    char              *url         = NULL;
    char              *data        = NULL;
    char              *e_owner     = NULL;
    char              *e_repo      = NULL;

    e_owner = gcli_urlencode(owner);
    e_repo  = gcli_urlencode(repo);

    url  = sn_asprintf(
        "%s/repos/%s/%s/pulls/%d",
        gcli_get_apibase(),
        e_owner, e_repo, pr_number);
    data = sn_asprintf("{ \"state\": \"open\"}");

    gcli_fetch_with_method("PATCH", url, data, NULL, &json_buffer);

    free(json_buffer.data);
    free(url);
    free(data);
    free(e_owner);
    free(e_repo);
}

void
github_perform_submit_pr(gcli_submit_pull_options opts)
{
    sn_sv              e_head, e_base, e_title, e_body;
    gcli_fetch_buffer  fetch_buffer = {0};
    struct json_stream json         = {0};
    gcli_pull_summary  pull         = {0};

    e_head  = gcli_json_escape(opts.from);
    e_base  = gcli_json_escape(opts.to);
    e_title = gcli_json_escape(opts.title);
    e_body  = gcli_json_escape(opts.body);

    char *post_fields = sn_asprintf(
        "{\"head\":\""SV_FMT"\",\"base\":\""SV_FMT"\", "
        "\"title\": \""SV_FMT"\", \"body\": \""SV_FMT"\" }",
        SV_ARGS(e_head),
        SV_ARGS(e_base),
        SV_ARGS(e_title),
        SV_ARGS(e_body));
    char *url         = sn_asprintf(
        "%s/repos/"SV_FMT"/"SV_FMT"/pulls",
        gcli_get_apibase(),
        SV_ARGS(opts.owner), SV_ARGS(opts.repo));

    gcli_fetch_with_method("POST", url, post_fields, NULL, &fetch_buffer);

    /* Add labels if requested. GitHub doesn't allow us to do this all
     * with one request. */
    if (opts.labels_size) {
        json_open_buffer(&json, fetch_buffer.data, fetch_buffer.length);
        parse_github_pull_summary(&json, &pull);

        /* HACK: the string in the string view is passed from the
         *       command line and is thus null-terminated. We don't
         *       need to convert it into a C-string. Ideally,
         *       gcli_pulls_create_options.(repo|owner) should be a
         *       C-string to begin with. */
        github_issue_add_labels(opts.owner.data, opts.repo.data,
                                pull.id, opts.labels, opts.labels_size);

        gcli_pulls_summary_free(&pull);
        json_close(&json);
    }

    free(fetch_buffer.data);
    free(e_head.data);
    free(e_base.data);
    free(e_title.data);
    free(e_body.data);
    free(post_fields);
    free(url);
}

int
github_get_pull_commits(
    const char   *owner,
    const char   *repo,
    int           pr_number,
    gcli_commit **out)
{
    char              *url         = NULL;
    char              *next_url    = NULL;
    char              *e_owner     = NULL;
    char              *e_repo      = NULL;
    size_t             count       = 0;
    json_stream        stream      = {0};
    gcli_fetch_buffer  json_buffer = {0};

    e_owner = gcli_urlencode(owner);
    e_repo  = gcli_urlencode(repo);

    url = sn_asprintf(
        "%s/repos/%s/%s/pulls/%d/commits",
        gcli_get_apibase(),
        e_owner, e_repo, pr_number);

    do {
        gcli_fetch(url, &next_url, &json_buffer);
        json_open_buffer(&stream, json_buffer.data, json_buffer.length);

        parse_github_commits(&stream, out, &count);

        json_close(&stream);
        free(json_buffer.data);
        free(url);
    } while ((url = next_url));

    free(e_owner);
    free(e_repo);

    return (int)count;
}

void
github_get_pull_summary(
    const char        *owner,
    const char        *repo,
    int                pr_number,
    gcli_pull_summary *out)
{
    json_stream        stream      = {0};
    gcli_fetch_buffer  json_buffer = {0};
    char              *url         = NULL;
    char              *e_owner     = NULL;
    char              *e_repo      = NULL;

    e_owner = gcli_urlencode(owner);
    e_repo  = gcli_urlencode(repo);

    url = sn_asprintf(
        "%s/repos/%s/%s/pulls/%d",
        gcli_get_apibase(),
        e_owner, e_repo, pr_number);
    gcli_fetch(url, NULL, &json_buffer);

    json_open_buffer(&stream, json_buffer.data, json_buffer.length);

    parse_github_pull_summary(&stream, out);

    json_close(&stream);
    free(url);
    free(e_owner);
    free(e_repo);
    free(json_buffer.data);
}
