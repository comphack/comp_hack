/**
 * @file tools/cathedral/src/ObjectListModel.h
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Implementation for a data model that lists objgen objects.
 *
 * Copyright (C) 2012-2018 COMP_hack Team <compomega@tutanota.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ObjectListModel.h"

// Cathedral Includes
#include "ObjectListWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>

ObjectListModel::ObjectListModel(ObjectListWindow *pListWindow,
    QObject *pParent) : QAbstractListModel(pParent), mListWindow(pListWindow)
{
}

ObjectListModel::~ObjectListModel()
{
}

void ObjectListModel::SetObjectList(const std::vector<
    std::shared_ptr<libcomp::Object>>& objs)
{
    beginResetModel();
    mObjects = objs;
    endResetModel();
}

std::shared_ptr<libcomp::Object> ObjectListModel::GetObject(
    const QModelIndex& index) const
{
    auto row = index.row();

    if((int)mObjects.size() > row)
    {
        return mObjects[(std::vector<std::shared_ptr<
            libcomp::Object>>::size_type)row];
    }

    return {};
}

int ObjectListModel::rowCount(const QModelIndex& parent) const
{
    if(!parent.isValid())
    {
        return (int)mObjects.size();
    }

    return 0;
}

QVariant ObjectListModel::data(const QModelIndex& index, int role) const
{
    auto row = index.row();

    if((int)mObjects.size() > row && Qt::DisplayRole == role)
    {
        auto obj = mObjects[(std::vector<std::shared_ptr<
            libcomp::Object>>::size_type)row];

        QString id = mListWindow->GetObjectID(obj);
        QString name = mListWindow->GetObjectName(obj);

        if(name.isEmpty())
        {
            return id;
        }
        else
        {
            return QString("%1 (%2)").arg(id).arg(name);
        }
    }

    return {};
}
