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

#include <ghcli-qt/issuemodel.hh>

namespace ghcli
{

    enum class IssueIndex {
        Title  = 0,
        State  = 1,
        Number = 2,
        Id     = 3,
    };

    IssueModel::IssueModel(ghcli_issue *issues, size_t issues_size)
        : QAbstractTableModel(nullptr)
        , m_issues(issues)
        , m_issues_size(issues_size)
    {
    }

    IssueModel::~IssueModel()
    {
        free(this->m_issues);
    }

    QVariant IssueModel::data(const QModelIndex &index, int role) const
    {
        switch (role) {
        case Qt::DisplayRole: {
            switch ((IssueIndex)(index.column()))
            {
            case IssueIndex::Title:
                return { this->m_issues[index.row()].title };
            case IssueIndex::State:
                return { this->m_issues[index.row()].state };
            case IssueIndex::Number:
                return { this->m_issues[index.row()].number };
            case IssueIndex::Id:
                return { this->m_issues[index.row()].id };
            default:
                return { "WTF" };
            }
        } break;
        default:
            return {};
        }
    }

    QVariant IssueModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (role != Qt::DisplayRole)
            return {};

        if (orientation == Qt::Horizontal) {
            switch ((IssueIndex)section) {
            case IssueIndex::Title:
                return "Title";
            case IssueIndex::State:
                return "State";
            case IssueIndex::Number:
                return "Number";
            case IssueIndex::Id:
                return "Id";
            default:
                return "WTF";
            }
        } else {
            return {};
        }
    }

    int IssueModel::columnCount(const QModelIndex &) const
    {
        return 4;
    }

    int IssueModel::rowCount(const QModelIndex &) const
    {
        return (int)this->m_issues_size;
    }


}
