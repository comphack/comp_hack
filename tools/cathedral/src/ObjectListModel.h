/**
 * @file tools/cathedral/src/ObjectListModel.h
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Definition for a data model that lists objgen objects.
 *
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
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

#ifndef TOOLS_CATHEDRAL_SRC_OBJECTLISTMODEL_H
#define TOOLS_CATHEDRAL_SRC_OBJECTLISTMODEL_H

// Qt Includes
#include <PushIgnore.h>
#include <QAbstractListModel>
#include <PopIgnore.h>

// objects Includes
#include <Object.h>

class ObjectList;

class ObjectListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit ObjectListModel(ObjectList *pList, QObject *pParent = 0);
    virtual ~ObjectListModel();

    virtual void SetObjectList(const std::vector<
        std::shared_ptr<libcomp::Object>>& objs);

    int GetIndex(const std::shared_ptr<libcomp::Object>& obj);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index,
        int role = Qt::DisplayRole) const override;

    std::shared_ptr<libcomp::Object> GetObject(const QModelIndex& index) const;

protected:
    ObjectList *mList;
    std::vector<std::shared_ptr<libcomp::Object>> mObjects;
};

#endif // TOOLS_CATHEDRAL_SRC_OBJECTLISTMODEL_H
