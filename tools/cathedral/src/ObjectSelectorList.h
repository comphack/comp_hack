/**
 * @file tools/cathedral/src/ObjectSelectorList.h
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Definition for a value selection window bound to an ObjectSelector.
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

#ifndef TOOLS_CATHEDRAL_SRC_OBJECTSELECTORLIST_H
#define TOOLS_CATHEDRAL_SRC_OBJECTSELECTORLIST_H

#include "ObjectList.h"

class BinaryDataNamedSet;

class ObjectSelectorList : public ObjectList
{
    Q_OBJECT

public:
    explicit ObjectSelectorList(const std::shared_ptr<
        BinaryDataNamedSet>& dataSet, bool emptySelectable,
        QWidget *pParent = 0);
    virtual ~ObjectSelectorList();
    
    QString GetObjectID(const std::shared_ptr<
        libcomp::Object>& obj) const override;

    QString GetObjectName(const std::shared_ptr<
        libcomp::Object>& obj) const override;

    bool Select(uint32_t value);

    void LoadIfNeeded();

    void LoadProperties(const std::shared_ptr<libcomp::Object>& obj) override;

    std::shared_ptr<libcomp::Object> GetSelectedObject();

signals:
    void selectedObjectChanged();

private:
    std::shared_ptr<BinaryDataNamedSet> mDataSet;

    bool mEmptySelectable;

    bool mLoaded;
};

#endif // TOOLS_CATHEDRAL_SRC_OBJECTSELECTORLIST_H
