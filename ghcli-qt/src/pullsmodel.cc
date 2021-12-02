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

#include <ghcli-qt/pullsmodel.hh>

namespace ghcli
{

    enum class PullIndex
    {
        Title = 0,
        State = 1,
        Creator = 2,
        Number = 3,
        Id = 4,
        Merged = 5,
    };


    PullsModel::PullsModel(ghcli_pull *pulls, size_t pulls_size)
        : QAbstractTableModel(nullptr)
        , m_pulls(pulls)
        , m_pulls_size(pulls_size)
    {
    }

    PullsModel::~PullsModel()
    {
        ghcli_pulls_free(this->m_pulls, this->m_pulls_size);
    }

    QVariant PullsModel::data(const QModelIndex &index, int role) const
    {
        if (role == Qt::DisplayRole)
        {
            int row = index.row();

            switch ((PullIndex)index.column())
            {
            case PullIndex::Title:
                return this->m_pulls[row].title;
            case PullIndex::State:
                return this->m_pulls[row].state;
            case PullIndex::Creator:
                return this->m_pulls[row].creator;
            case PullIndex::Number:
                return this->m_pulls[row].number;
            case PullIndex::Id:
                return this->m_pulls[row].id;
            case PullIndex::Merged:
                return this->m_pulls[row].merged;
            default:
                return "wtf";
            }
        }

        return {};
    }

    QVariant PullsModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
        {
            switch ((PullIndex)section)
            {
            case PullIndex::Title:
                return "Title";
            case PullIndex::State:
                return "State";
            case PullIndex::Creator:
                return "Creator";
            case PullIndex::Number:
                return "Number";
            case PullIndex::Id:
                return "Id";
            case PullIndex::Merged:
                return "Merged";
            default:
                return "wtf";
            }
        }

        return {};
    }

    int PullsModel::columnCount(const QModelIndex &) const
    {
        return 6;
    }

    int PullsModel::rowCount(const QModelIndex &) const
    {
        return this->m_pulls_size;
    }

} // ghcli
