Keywords: gitlab pipelines ci
ID: 12
Title: GitLab Pipeline with Trigger Jobs

# build/gcli -t gitlab pi -o gitlab-org -r terraform-provider-gitlab -p 2235706226 all
ClientArgs: -t gitlab pi -o gitlab-org -r terraform-provider-gitlab -p 2235706226 jobs children
VerifyClientExitCode: 0
VerifyClientOutput:
  ID           NAME                     STATUS   STARTED               FINISHED              REF
  12556302952  testacc:nightly:failure  success  2025-Dec-29 04:06:31  2025-Dec-29 04:07:29  main
  
  ID          STATUS  CREATED               UPDATED               NAME                     REF
  2235713532  failed  2025-Dec-29 03:07:08  2025-Dec-29 04:06:30  testacc:nightly:failure  main

# First request
ServerResponseStatus: 200 OK

ServerResponseBody:
  [
  ]

VerifyRequestMethod: GET
VerifyRequestPath: /projects/gitlab-org%2Fterraform-provider-gitlab/pipelines/2235706226/jobs

# Second request
ServerResponseStatus: 200 OK

ServerResponseBody:
  [
    {
      "name": "testacc:nightly:failure",
      "id": 12556302952,
      "status": "success",
      "stage": "acceptance-test",
      "name": "testacc:nightly:failure",
      "ref": "main",
      "created_at": "2025-12-29T03:03:21.574Z",
      "started_at": "2025-12-29T04:06:31.948Z",
      "finished_at": "2025-12-29T04:07:29.137Z",
      "runner": { "name": null, "description": "wat" },
      "duration": 57.188417,
      "coverage": null,
      "web_url": "http://example.com",
      "downstream_pipeline": {
        "id": 2235713532,
        "status": "failed",
        "ref": "main",
        "web_url": "http://example.com",
        "created_at": "2025-12-29T03:07:08.602Z",
        "updated_at": "2025-12-29T04:06:30.037Z"
      }
    }
  ]

VerifyRequestMethod: GET
VerifyRequestPath: /projects/gitlab-org%2Fterraform-provider-gitlab/pipelines/2235706226/bridges

# Third request
ServerResponseStatus: 200 OK

ServerResponseBody:
  [
    {
      "name": "testacc:nightly:failure",
      "id": 12556302952,
      "status": "success",
      "stage": "acceptance-test",
      "name": "testacc:nightly:failure",
      "ref": "main",
      "created_at": "2025-12-29T03:03:21.574Z",
      "started_at": "2025-12-29T04:06:31.948Z",
      "finished_at": "2025-12-29T04:07:29.137Z",
      "runner": { "name": null, "description": "wat" },
      "duration": 57.188417,
      "coverage": null,
      "web_url": "http://example.com",
      "downstream_pipeline": {
        "id": 2235713532,
        "status": "failed",
        "ref": "main",
        "web_url": "http://example.com",
        "created_at": "2025-12-29T03:07:08.602Z",
        "updated_at": "2025-12-29T04:06:30.037Z"
      }
    }
  ]

VerifyRequestMethod: GET
VerifyRequestPath: /projects/gitlab-org%2Fterraform-provider-gitlab/pipelines/2235706226/bridges

