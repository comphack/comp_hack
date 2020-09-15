/**
 * @file tools/cathedral/src/ObjectSelector.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for a value bound to an object with a selectable
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

#include "ObjectSelector.h"

// Cathedral Includes
#include "BinaryDataNamedSet.h"
#include "EventWindow.h"
#include "MainWindow.h"
#include "ObjectSelectorWindow.h"

// Qt Includes
#include <PopIgnore.h>
#include <PushIgnore.h>

#include <QLineEdit>

#include "ui_ObjectSelector.h"

ObjectSelector::ObjectSelector(QWidget *pParent)
    : ObjectSelectorBase(pParent), mServerData(false) {
  ui = new Ui::ObjectSelector;
  ui->setupUi(this);

  connect(ui->getItem, SIGNAL(clicked()), this, SLOT(GetItem()));
  connect(ui->value, SIGNAL(valueChanged(int)), this, SLOT(ValueChanged()));
}

ObjectSelector::~ObjectSelector() { delete ui; }

bool ObjectSelector::BindSelector(MainWindow *pMainWindow,
                                  const libcomp::String &objType,
                                  bool serverData) {
  if (mObjType != objType) {
    bool changed = Bind(pMainWindow, objType);

    mServerData = serverData;

    ValueChanged();

    return changed;
  }

  return false;
}

void ObjectSelector::SetValue(uint32_t value) {
  ui->value->setValue((int32_t)value);
}

uint32_t ObjectSelector::GetValue() const {
  return (uint32_t)ui->value->value();
}

void ObjectSelector::SetValueSigned(int32_t value) {
  ui->value->setValue(value);
}

int32_t ObjectSelector::GetValueSigned() const { return ui->value->value(); }

void ObjectSelector::SetMinimum(int32_t min) { ui->value->setMinimum(min); }

void ObjectSelector::ValueChanged() {
  uint32_t value = GetValue();

  QString txt =
      qs(value ? (mServerData ? "[Not loaded]" : "[Invalid]") : "[None]");
  if (mMainWindow && value) {
    auto dataset = std::dynamic_pointer_cast<BinaryDataNamedSet>(
        mMainWindow->GetBinaryDataSet(mObjType));
    auto obj = dataset ? dataset->GetObjectByID(value) : nullptr;
    if (obj) {
      txt = qs(dataset->GetName(obj)
                   .Replace("\n", "  ")
                   .Replace("\r", "  ")
                   .Replace("    ", "  "));
    }
  }

  ui->label->setText(txt);
}
