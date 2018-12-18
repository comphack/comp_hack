/**
 * @file tools/cathedral/src/EventPerformActionsUI.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for a perform actions event.
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

#include "EventPerformActionsUI.h"

// Cathedral Includes
#include "MainWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include <QLineEdit>

#include "ui_Event.h"
#include "ui_EventPerformActions.h"
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

EventPerformActions::EventPerformActions(MainWindow *pMainWindow,
    QWidget *pParent) : Event(pMainWindow, pParent)
{
    QWidget *pWidget = new QWidget;
    prop = new Ui::EventPerformActions;
    prop->setupUi(pWidget);

    ui->eventTitle->setText(tr("<b>Perform Actions</b>"));
    ui->layoutMain->addWidget(pWidget);
}

EventPerformActions::~EventPerformActions()
{
    delete prop;
}

void EventPerformActions::Load(const std::shared_ptr<objects::Event>& e)
{
    Event::Load(e);

    mEvent = std::dynamic_pointer_cast<objects::EventPerformActions>(e);

    if(!mEvent)
    {
        return;
    }

    /// @todo
}

std::shared_ptr<objects::Event> EventPerformActions::Save() const
{
    return mEvent;
}
