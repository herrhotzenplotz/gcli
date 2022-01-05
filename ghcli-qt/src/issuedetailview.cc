/*
 * Copyright 2022 Nico Sonack <nsonack@outlook.com>
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

#include <ghcli-qt/issuedetailview.hh>

#include <QFuture>
#include <QtConcurrent>
#include <QFormLayout>

namespace ghcli
{

    IssueDetailView::IssueDetailView(const char *owner, const char *repo, int issue)
    {
        connect(&m_fetch_watcher, &QFutureWatcher<void>::finished, this, &IssueDetailView::fetchFinished);
        auto future = QtConcurrent::run(ghcli_get_issue_details, owner, repo, issue, &m_details);
        m_fetch_watcher.setFuture(future);
    }

    IssueDetailView::~IssueDetailView()
    {
        ghcli_issue_details_free(&m_details);
    }

    void IssueDetailView::fetchFinished()
    {
        auto *layout = new QFormLayout(this);
        layout->addRow("Title",  new QLabel { QString::fromUtf8(m_details.title.data, m_details.title.length) });
        layout->addRow("Author", new QLabel { QString::fromUtf8(m_details.author.data, m_details.author.length) });
    }

}
