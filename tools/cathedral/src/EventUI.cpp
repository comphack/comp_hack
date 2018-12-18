/**
 * @file tools/cathedral/src/EventUI.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for an event.
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

#include "EventUI.h"

// Cathedral Includes
#include "MainWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include "ui_Event.h"
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

Event::Event(MainWindow *pMainWindow, QWidget *pParent) :
    QWidget(pParent), mMainWindow(pMainWindow)
{
    ui = new Ui::Event;
    ui->setupUi(this);

    ui->eventTitle->setText(tr("<b>Fork</b>"));
}

Event::~Event()
{
    delete ui;
}

void Event::Load(const std::shared_ptr<objects::Event>& e)
{
    mEventBase = e;

    if(!mEventBase)
    {
        return;
    }

    /// @todo
}

std::shared_ptr<objects::Event> Event::Save() const
{
    return mEventBase;
}
