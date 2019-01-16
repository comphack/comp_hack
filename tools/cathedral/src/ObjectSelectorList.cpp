/**
 * @file tools/cathedral/src/ObjectSelectorList.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for a value selection window bound to an
 *  ObjectSelector.
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

#include "ObjectSelectorList.h"

// Cathedral Includes
#include "BinaryDataNamedSet.h"
#include "MainWindow.h"
#include "ObjectListModel.h"

ObjectSelectorList::ObjectSelectorList(const std::shared_ptr<
    BinaryDataNamedSet>& dataSet, const libcomp::String& objType,
    bool emtpySelectable, QWidget *pParent) : ObjectList(pParent),
    mDataSet(dataSet), mObjType(objType), mEmptySelectable(emtpySelectable),
    mLoaded(false)
{
}

ObjectSelectorList::~ObjectSelectorList()
{
}


QString ObjectSelectorList::GetObjectID(const std::shared_ptr<
    libcomp::Object>& obj) const
{
    if(mDataSet)
    {
        return QString::number(mDataSet->GetMapID(obj));
    }

    return "0";
}

QString ObjectSelectorList::GetObjectName(const std::shared_ptr<
    libcomp::Object>& obj) const
{
    if(mDataSet)
    {
        // "Tab in" new lines so they are easier to read
        return qs(mDataSet->GetName(obj).Replace("\r", "\n")
            .Replace("\n\n", "\n").Replace("\n", "\n\r    "));
    }

    return "";
}

bool ObjectSelectorList::Select(uint32_t value)
{
    if(mDataSet)
    {
        auto obj = mDataSet->GetObjectByID(value);
        return obj && ObjectList::Select(obj);
    }

    return false;
}

void ObjectSelectorList::LoadIfNeeded()
{
    if(!mLoaded)
    {
        std::vector<std::shared_ptr<libcomp::Object>> objs;
        if(mDataSet)
        {
            for(auto obj : mDataSet->GetObjects())
            {
                if(mEmptySelectable || !GetObjectName(obj).isEmpty())
                {
                    objs.push_back(obj);
                }
            }
        }

        SetObjectList(objs);

        mLoaded = true;
    }
}

libcomp::String ObjectSelectorList::GetObjectType() const
{
    return mObjType;
}

std::shared_ptr<libcomp::Object> ObjectSelectorList::GetSelectedObject()
{
    return mActiveObject.lock();
}
