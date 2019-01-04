/**
 * @file tools/cathedral/src/ObjectSelectorBase.h
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Definition for a field bound to an object with a selectable
 *  text representation.
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

#ifndef TOOLS_CATHEDRAL_SRC_OBJECTSELECTORBASE_H
#define TOOLS_CATHEDRAL_SRC_OBJECTSELECTORBASE_H

// Qt Includes
#include <PushIgnore.h>
#include <QWidget>
#include <PopIgnore.h>

// libcomp Includes
#include <CString.h>

class MainWindow;

class ObjectSelectorBase : public QWidget
{
    Q_OBJECT

public:
    explicit ObjectSelectorBase(QWidget *pParent = 0);
    virtual ~ObjectSelectorBase();

    virtual bool Bind(MainWindow *pMainWindow, const libcomp::String& objType);

    virtual void SetValue(uint32_t value) = 0;
    virtual uint32_t GetValue() const = 0;

    libcomp::String GetObjectType() const;

protected slots:
    void GetItem();

protected:
    MainWindow *mMainWindow;

    libcomp::String mObjType;
};

#endif // TOOLS_CATHEDRAL_SRC_OBJECTSELECTORBASE_H
