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

#include <ghcli-qt/issueview.hh>

#include <QtWidgets>

namespace ghcli
{

    IssueView::IssueView(const char *org, const char *repo, QWidget *parent, bool all)
        : QTableView(parent)
        , m_model(nullptr)
    {
        ghcli_issue *issues = nullptr;
        int issues_size = ghcli_get_issues(org, repo, all, -1, &issues);
        this->m_model = new IssueModel(issues, issues_size);

        this->setModel(this->m_model);
        this->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

        connect(this, &QAbstractItemView::doubleClicked,
                this, &IssueView::onIssueDoubleClicked);
    }

    IssueView::~IssueView()
    {
        if (this->m_model)
            delete this->m_model;
    }

    void IssueView::onIssueDoubleClicked(const QModelIndex& idx)
    {
        auto& issue = m_model->getIssue(idx.row());
        emit issueDoubleClicked(issue.number);
    }


} // ghcli