/**
 * @file tools/cathedral/src/DynamicListItem.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for a dynamic list item.
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

#include "DynamicListItem.h"

// Qt Includes
#include <PushIgnore.h>
#include "ui_DynamicListItem.h"
#include <PopIgnore.h>

DynamicListItem::DynamicListItem(QWidget *pParent) : QWidget(pParent)
{
    ui = new Ui::DynamicListItem;
    ui->setupUi(this);
}

DynamicListItem::~DynamicListItem()
{
    delete ui;
}
