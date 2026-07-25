Keywords: github repos
ID: 32
Title: GitHub list repositories of organisation

ClientArgs: -t github repos -o herrhotzenplotz
VerifyClientExitCode: 0
VerifyClientOutput:
  FORK  VISBLTY  DATE                  FULLNAME
  no    public   2021-Oct-08 14:20:15  herrhotzenplotz/gcli

# first request: determine whether this is a user or an org
VerifyRequestMethod: GET
VerifyRequestPath: /users/herrhotzenplotz

ServerResponseStatus: 404 Not found
ServerResponseBody:
  {}

# second request: list the organisations repositories
VerifyRequestMethod: GET
VerifyRequestPath: /orgs/herrhotzenplotz/repos

ServerResponseStatus: 200 OK
ServerResponseBody:
  [
    {
      "id": 415015197,
      "node_id": "R_kgDOGLyhHQ",
      "name": "gcli",
      "full_name": "herrhotzenplotz/gcli",
      "private": false,
      "owner": {
        "login": "herrhotzenplotz",
        "id": 34663024,
        "node_id": "MDQ6VXNlcjM0NjYzMDI0",
        "avatar_url": "https://avatars.githubusercontent.com/u/34663024?v=4",
        "gravatar_id": "",
        "url": "https://api.github.com/users/herrhotzenplotz",
        "html_url": "https://github.com/herrhotzenplotz",
        "followers_url": "https://api.github.com/users/herrhotzenplotz/followers",
        "following_url": "https://api.github.com/users/herrhotzenplotz/following{/other_user}",
        "gists_url": "https://api.github.com/users/herrhotzenplotz/gists{/gist_id}",
        "starred_url": "https://api.github.com/users/herrhotzenplotz/starred{/owner}{/repo}",
        "subscriptions_url": "https://api.github.com/users/herrhotzenplotz/subscriptions",
        "organizations_url": "https://api.github.com/users/herrhotzenplotz/orgs",
        "repos_url": "https://api.github.com/users/herrhotzenplotz/repos",
        "events_url": "https://api.github.com/users/herrhotzenplotz/events{/privacy}",
        "received_events_url": "https://api.github.com/users/herrhotzenplotz/received_events",
        "type": "User",
        "user_view_type": "public",
        "site_admin": false
      },
      "html_url": "https://github.com/herrhotzenplotz/gcli",
      "description": "Portable Git(hub|lab|tea)/Forgejo/Bugzilla CLI tool, Submit patches here: https://lists.sr.ht/~herrhotzenplotz/gcli-devel",
      "fork": false,
      "url": "https://api.github.com/repos/herrhotzenplotz/gcli",
      "forks_url": "https://api.github.com/repos/herrhotzenplotz/gcli/forks",
      "keys_url": "https://api.github.com/repos/herrhotzenplotz/gcli/keys{/key_id}",
      "collaborators_url": "https://api.github.com/repos/herrhotzenplotz/gcli/collaborators{/collaborator}",
      "teams_url": "https://api.github.com/repos/herrhotzenplotz/gcli/teams",
      "hooks_url": "https://api.github.com/repos/herrhotzenplotz/gcli/hooks",
      "issue_events_url": "https://api.github.com/repos/herrhotzenplotz/gcli/issues/events{/number}",
      "events_url": "https://api.github.com/repos/herrhotzenplotz/gcli/events",
      "assignees_url": "https://api.github.com/repos/herrhotzenplotz/gcli/assignees{/user}",
      "branches_url": "https://api.github.com/repos/herrhotzenplotz/gcli/branches{/branch}",
      "tags_url": "https://api.github.com/repos/herrhotzenplotz/gcli/tags",
      "blobs_url": "https://api.github.com/repos/herrhotzenplotz/gcli/git/blobs{/sha}",
      "git_tags_url": "https://api.github.com/repos/herrhotzenplotz/gcli/git/tags{/sha}",
      "git_refs_url": "https://api.github.com/repos/herrhotzenplotz/gcli/git/refs{/sha}",
      "trees_url": "https://api.github.com/repos/herrhotzenplotz/gcli/git/trees{/sha}",
      "statuses_url": "https://api.github.com/repos/herrhotzenplotz/gcli/statuses/{sha}",
      "languages_url": "https://api.github.com/repos/herrhotzenplotz/gcli/languages",
      "stargazers_url": "https://api.github.com/repos/herrhotzenplotz/gcli/stargazers",
      "contributors_url": "https://api.github.com/repos/herrhotzenplotz/gcli/contributors",
      "subscribers_url": "https://api.github.com/repos/herrhotzenplotz/gcli/subscribers",
      "subscription_url": "https://api.github.com/repos/herrhotzenplotz/gcli/subscription",
      "commits_url": "https://api.github.com/repos/herrhotzenplotz/gcli/commits{/sha}",
      "git_commits_url": "https://api.github.com/repos/herrhotzenplotz/gcli/git/commits{/sha}",
      "comments_url": "https://api.github.com/repos/herrhotzenplotz/gcli/comments{/number}",
      "issue_comment_url": "https://api.github.com/repos/herrhotzenplotz/gcli/issues/comments{/number}",
      "contents_url": "https://api.github.com/repos/herrhotzenplotz/gcli/contents/{+path}",
      "compare_url": "https://api.github.com/repos/herrhotzenplotz/gcli/compare/{base}...{head}",
      "merges_url": "https://api.github.com/repos/herrhotzenplotz/gcli/merges",
      "archive_url": "https://api.github.com/repos/herrhotzenplotz/gcli/{archive_format}{/ref}",
      "downloads_url": "https://api.github.com/repos/herrhotzenplotz/gcli/downloads",
      "issues_url": "https://api.github.com/repos/herrhotzenplotz/gcli/issues{/number}",
      "pulls_url": "https://api.github.com/repos/herrhotzenplotz/gcli/pulls{/number}",
      "milestones_url": "https://api.github.com/repos/herrhotzenplotz/gcli/milestones{/number}",
      "notifications_url": "https://api.github.com/repos/herrhotzenplotz/gcli/notifications{?since,all,participating}",
      "labels_url": "https://api.github.com/repos/herrhotzenplotz/gcli/labels{/name}",
      "releases_url": "https://api.github.com/repos/herrhotzenplotz/gcli/releases{/id}",
      "deployments_url": "https://api.github.com/repos/herrhotzenplotz/gcli/deployments",
      "created_at": "2021-10-08T14:20:15Z",
      "updated_at": "2026-07-25T07:12:51Z",
      "pushed_at": "2026-07-25T07:12:13Z",
      "git_url": "git://github.com/herrhotzenplotz/gcli.git",
      "ssh_url": "git@github.com:herrhotzenplotz/gcli.git",
      "clone_url": "https://github.com/herrhotzenplotz/gcli.git",
      "svn_url": "https://github.com/herrhotzenplotz/gcli",
      "homepage": "https://sr.ht/~herrhotzenplotz/gcli/",
      "size": 6339,
      "stargazers_count": 176,
      "watchers_count": 176,
      "language": "C",
      "has_issues": true,
      "has_projects": false,
      "has_downloads": false,
      "has_wiki": false,
      "has_pages": false,
      "has_discussions": false,
      "forks_count": 8,
      "mirror_url": null,
      "archived": false,
      "disabled": false,
      "open_issues_count": 4,
      "license": {
        "key": "bsd-2-clause",
        "name": "BSD 2-Clause \"Simplified\" License",
        "spdx_id": "BSD-2-Clause",
        "url": "https://api.github.com/licenses/bsd-2-clause",
        "node_id": "MDc6TGljZW5zZTQ="
      },
      "allow_forking": true,
      "is_template": false,
      "web_commit_signoff_required": false,
      "has_pull_requests": false,
      "pull_request_creation_policy": "all",
      "topics": [
        "c",
        "cli",
        "freebsd",
        "github-api",
        "gitlab",
        "gitlab-api",
        "libcurl",
        "linux",
        "open-source",
        "terminal",
        "unix"
      ],
      "visibility": "public",
      "forks": 8,
      "open_issues": 4,
      "watchers": 176,
      "default_branch": "trunk",
      "permissions": {
        "admin": true,
        "maintain": true,
        "push": true,
        "triage": true,
        "pull": true
      },
      "score": 1.0
    }
  ]

