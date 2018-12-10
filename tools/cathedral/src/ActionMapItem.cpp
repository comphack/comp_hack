/**
 * @file tools/cathedral/src/ActionMapItem.cpp
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Implementation for an item in an action map.
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

#include "ActionMapItem.h"

// Cathedral Includes
#include "ActionMap.h"

// Qt Includes
#include <PushIgnore.h>
#include <QLineEdit>

#include "ui_ActionMapItem.h"
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

// Standard C++11 Includes
#include <algorithm>

ActionMapItem::ActionMapItem(const QString& valueName, ActionMap *pMap,
    uint32_t key, int32_t value, QWidget *pParent) : QWidget(pParent),
    mMap(pMap)
{
    ui = new Ui::ActionMapItem;
    ui->setupUi(this);

    ui->key->lineEdit()->setText(QString::number(key));
    ui->value->setValue(value);

    if(!valueName.isEmpty())
    {
        ui->valueLabel->setText(valueName);
    }

    connect(ui->remove, SIGNAL(clicked(bool)), this, SLOT(Remove()));
}

ActionMapItem::ActionMapItem(const QString& valueName, ActionMap *pMap,
    QWidget *pParent) : QWidget(pParent), mMap(pMap)
{
    ui = new Ui::ActionMapItem;
    ui->setupUi(this);

    if(!valueName.isEmpty())
    {
        ui->valueLabel->setText(valueName);
    }

    connect(ui->remove, SIGNAL(clicked(bool)), this, SLOT(Remove()));
}

ActionMapItem::~ActionMapItem()
{
    delete ui;
}

uint32_t ActionMapItem::GetKey() const
{
    /// @todo Implement
    return 0;
}

int32_t ActionMapItem::GetValue() const
{
    return ui->value->value();
}

void ActionMapItem::SetMinMax(int32_t min, int32_t max)
{
    ui->value->setMinimum(min);
    ui->value->setMaximum(max);
}

void ActionMapItem::Remove()
{
    if(mMap)
    {
        mMap->RemoveValue(this);
    }
}
