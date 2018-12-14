/**
 * @file tools/cathedral/src/ActionIDsItem.cpp
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

#include "ActionIDsItem.h"

// Cathedral Includes
#include "ActionIDs.h"

// Qt Includes
#include <PushIgnore.h>
#include <QLineEdit>

#include "ui_ActionIDsItem.h"
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

// Standard C++11 Includes
#include <algorithm>

ActionIDsItem::ActionIDsItem(ActionIDs *pIDs, uint32_t value,
    QWidget *pParent) : QWidget(pParent), mIDs(pIDs)
{
    ui = new Ui::ActionIDsItem;
    ui->setupUi(this);

    ui->item->lineEdit()->setText(QString::number(value));

    connect(ui->remove, SIGNAL(clicked(bool)), this, SLOT(Remove()));
}

ActionIDsItem::ActionIDsItem(ActionIDs *pIDs, QWidget *pParent) :
    QWidget(pParent), mIDs(pIDs)
{
    ui = new Ui::ActionIDsItem;
    ui->setupUi(this);

    connect(ui->remove, SIGNAL(clicked(bool)), this, SLOT(Remove()));
}

ActionIDsItem::~ActionIDsItem()
{
    delete ui;
}

uint32_t ActionIDsItem::GetValue() const
{
    QString txt = ui->item->currentText();

    bool valid = false;
    uint32_t val = txt.toInt(&valid);

    return valid ? val : 0;
}

void ActionIDsItem::Remove()
{
    if(mIDs)
    {
        mIDs->RemoveValue(this);
    }
}
