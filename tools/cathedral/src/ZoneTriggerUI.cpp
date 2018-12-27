/**
 * @file tools/cathedral/src/ZoneTriggerUI.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for a configured ZoneTrigger.
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

#include "ZoneTriggerUI.h"

// objects Includes
#include <ServerZoneTrigger.h>

// Qt Includes
#include <PushIgnore.h>
#include "ui_ZoneTrigger.h"
#include <PopIgnore.h>

// libcomp Includes
#include <PacketCodes.h>

ZoneTrigger::ZoneTrigger(MainWindow *pMainWindow,
    QWidget *pParent) : QWidget(pParent)
{
    prop = new Ui::ZoneTrigger;
    prop->setupUi(this);

    prop->actions->SetMainWindow(pMainWindow);
}

ZoneTrigger::~ZoneTrigger()
{
    delete prop;
}

void ZoneTrigger::Load(const std::shared_ptr<
    objects::ServerZoneTrigger>& trigger)
{
    prop->trigger->setCurrentIndex(to_underlying(
        trigger->GetTrigger()));
    prop->value->setValue(trigger->GetValue());

    auto actions = trigger->GetActions();
    prop->actions->Load(actions);
}

std::shared_ptr<objects::ServerZoneTrigger> ZoneTrigger::Save() const
{
    auto obj = std::make_shared<objects::ServerZoneTrigger>();

    obj->SetTrigger((objects::ServerZoneTrigger::Trigger_t)
        prop->trigger->currentIndex());
    obj->SetValue(prop->value->value());

    auto actions = prop->actions->Save();
    obj->SetActions(actions);

    return obj;
}
