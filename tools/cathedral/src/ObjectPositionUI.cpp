/**
 * @file tools/cathedral/src/ObjectPositionUI.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for a configured ObjectPosition.
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

#include "ObjectPositionUI.h"

// objects Includes
#include <ObjectPosition.h>

// Qt Includes
#include <PushIgnore.h>
#include <QLineEdit>

#include "ui_ObjectPosition.h"
#include <PopIgnore.h>

ObjectPosition::ObjectPosition(QWidget *pParent) : QWidget(pParent)
{
    prop = new Ui::ObjectPosition;
    prop->setupUi(this);

    connect(prop->radSpot, SIGNAL(clicked(bool)), this, SLOT(RadioToggle()));
    connect(prop->radPosition, SIGNAL(clicked(bool)), this, SLOT(RadioToggle()));
}

ObjectPosition::~ObjectPosition()
{
    delete prop;
}

void ObjectPosition::Load(const std::shared_ptr<objects::ObjectPosition>& pos)
{
    Load(pos->GetSpotID(), pos->GetX(), pos->GetY(), pos->GetRotation());
}

void ObjectPosition::Load(uint32_t spotID, float x, float y, float rot)
{
    prop->spot->lineEdit()->setText(
        QString::number(spotID));
    prop->x->setValue((double)x);
    prop->y->setValue((double)y);
    prop->rotation->setValue((double)rot);

    if(spotID || (!x && !y && !rot))
    {
        prop->radSpot->setChecked(true);
    }
    else
    {
        prop->radPosition->setChecked(true);
    }

    RadioToggle();
}

std::shared_ptr<objects::ObjectPosition> ObjectPosition::Save() const
{
    auto obj = std::make_shared<objects::ObjectPosition>();

    uint32_t spotID = (uint32_t)prop->spot->currentText().toInt();
    if(spotID)
    {
        obj->SetSpotID(spotID);
    }
    else
    {
        obj->SetX((float)prop->x->value());
        obj->SetY((float)prop->y->value());
        obj->SetRotation((float)prop->rotation->value());
    }

    return obj;
}

void ObjectPosition::RadioToggle()
{
    if(prop->radSpot->isChecked())
    {
        // Clear all position properties
        prop->x->setValue(0.0);
        prop->y->setValue(0.0);
        prop->rotation->setValue(0.0);

        prop->spot->setEnabled(true);
        prop->x->setEnabled(false);
        prop->y->setEnabled(false);
        prop->rotation->setEnabled(false);
    }
    else
    {
        // Clear spot property
        prop->spot->lineEdit()->setText("0");

        prop->spot->setEnabled(false);
        prop->x->setEnabled(true);
        prop->y->setEnabled(true);
        prop->rotation->setEnabled(true);
    }
}
