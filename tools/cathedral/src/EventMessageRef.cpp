/**
 * @file tools/cathedral/src/EventMessageRef.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for an event message being referenced.
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

#include "EventMessageRef.h"

// Cathedral Includes
#include "MainWindow.h"

// objects Includes
#include <MiCEventMessageData.h>

// Qt Includes
#include <PushIgnore.h>
#include "ui_EventMessageRef.h"
#include <PopIgnore.h>

EventMessageRef::EventMessageRef(QWidget *pParent) : QWidget(pParent),
    mMainWindow(nullptr)
{
    ui = new Ui::EventMessageRef;
    ui->setupUi(this);

    ui->message->setText("[Empty]");

    connect(ui->messageID, SIGNAL(valueChanged(int)), this,
        SLOT(MessageIDChanged()));
}

EventMessageRef::~EventMessageRef()
{
    delete ui;
}

void EventMessageRef::SetMainWindow(MainWindow *pMainWindow)
{
    mMainWindow = pMainWindow;
}

void EventMessageRef::SetValue(int32_t value)
{
    ui->messageID->setValue(value);
}

int32_t EventMessageRef::GetValue() const
{
    return ui->messageID->value();
}

void EventMessageRef::MessageIDChanged()
{
    ui->message->setText("[Empty]");

    if(mMainWindow)
    {
        auto msg = mMainWindow->GetEventMessage(GetValue());
        if(msg)
        {
            ui->message->setText(qs(
                libcomp::String::Join(msg->GetLines(), "\n")));
        }
    }
}
