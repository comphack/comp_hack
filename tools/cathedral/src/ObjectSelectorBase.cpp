/**
 * @file tools/cathedral/src/ObjectSelectorBase.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for a field bound to an object with a selectable
 *  text representation.
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

#include "ObjectSelectorBase.h"

// Cathedral Includes
#include "MainWindow.h"
#include "ObjectSelectorWindow.h"

// Qt Includes
#include <PopIgnore.h>
#include <PushIgnore.h>

#include <QLineEdit>
#include <QMessageBox>

#include "ui_ObjectSelector.h"

ObjectSelectorBase::ObjectSelectorBase(QWidget *pParent)
    : QWidget(pParent), mMainWindow(0) {}

ObjectSelectorBase::~ObjectSelectorBase() {}

bool ObjectSelectorBase::Bind(MainWindow *pMainWindow,
                              const libcomp::String &objType) {
  mMainWindow = pMainWindow;

  if (mObjType != objType) {
    bool changed = !mObjType.IsEmpty();

    mObjType = objType;

    return changed;
  }

  return false;
}

libcomp::String ObjectSelectorBase::GetObjectType() const { return mObjType; }

void ObjectSelectorBase::GetItem() {
  if (mMainWindow && !mObjType.IsEmpty()) {
    auto selector = mMainWindow->GetObjectSelector(mObjType);
    if (selector) {
      selector->Open(this);
    }
  }
}
