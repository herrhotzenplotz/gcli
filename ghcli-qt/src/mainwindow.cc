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

#include <ghcli-qt/mainwindow.hh>
#include <ghcli-qt/issuedetailview.hh>

#include <ghcli/gitconfig.h>

namespace ghcli
{
    MainWindow::MainWindow()
        : QMainWindow()
        , m_tabwidget(new QTabWidget {this})
        , m_issues(nullptr)
        , m_pulls(nullptr)
    {
        const char *org, *repo;
        ghcli_gitconfig_get_repo(&org, &repo);

        this->m_pulls  = new PullsView {org, repo, this, true};
        this->m_issues = new IssueView {org, repo, this, true};

        this->m_tabwidget->addTab(this->m_issues, "Issues");
        this->m_tabwidget->addTab(this->m_pulls, "PRs");

        this->setCentralWidget(this->m_tabwidget);

        connect(this->m_issues, &IssueView::issueDoubleClicked, this, &MainWindow::issueClicked);
    }

    MainWindow::~MainWindow()
    {
        if (this->m_tabwidget)
            delete this->m_tabwidget;
    }

    void MainWindow::issueClicked(int issue)
    {
        const char *owner, *repo;
        ghcli_gitconfig_get_repo(&owner, &repo);

        auto *details = new IssueDetailView {owner, repo, issue};
        this->m_tabwidget->addTab(details, QString::asprintf("Issue #%d", issue));
        details->setFocus(Qt::TabFocusReason);
    }
}