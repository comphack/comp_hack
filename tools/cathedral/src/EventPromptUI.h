/**
 * @file tools/cathedral/src/EventPromptUI.h
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Definition for a prompt event.
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

#ifndef TOOLS_CATHEDRAL_SRC_EVENTPROMPTUI_H
#define TOOLS_CATHEDRAL_SRC_EVENTPROMPTUI_H

// Cathedral Includes
#include "EventUI.h"

// objects Includes
#include <EventPrompt.h>

// Qt Includes
#include <PushIgnore.h>
#include <QWidget>
#include <PopIgnore.h>

namespace Ui
{

class EventPrompt;

} // namespace Ui

class MainWindow;

class EventPrompt : public Event
{
    Q_OBJECT

public:
    explicit EventPrompt(MainWindow *pMainWindow, QWidget *pParent = 0);
    virtual ~EventPrompt();

    void Load(const std::shared_ptr<objects::Event>& e) override;
    std::shared_ptr<objects::Event> Save() const override;

protected:
    Ui::EventPrompt *prop;

    std::shared_ptr<objects::EventPrompt> mEvent;
};

#endif // TOOLS_CATHEDRAL_SRC_EVENTPROMPTUI_H
