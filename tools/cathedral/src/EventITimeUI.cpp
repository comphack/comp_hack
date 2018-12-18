/**
 * @file tools/cathedral/src/EventITimeUI.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for an I-Time event.
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

#include "EventITimeUI.h"

// Cathedral Includes
#include "MainWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include <QLineEdit>

#include "ui_Event.h"
#include "ui_EventITime.h"
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

EventITime::EventITime(MainWindow *pMainWindow, QWidget *pParent)
    : Event(pMainWindow, pParent)
{
    QWidget *pWidget = new QWidget;
    prop = new Ui::EventITime;
    prop->setupUi(pWidget);

    ui->eventTitle->setText(tr("<b>I-Time</b>"));
    ui->layoutMain->addWidget(pWidget);
}

EventITime::~EventITime()
{
    delete prop;
}

void EventITime::Load(const std::shared_ptr<objects::Event>& e)
{
    Event::Load(e);

    mEvent = std::dynamic_pointer_cast<objects::EventITime>(e);

    if(!mEvent)
    {
        return;
    }

    /// @todo
}

std::shared_ptr<objects::Event> EventITime::Save() const
{
    return mEvent;
}
