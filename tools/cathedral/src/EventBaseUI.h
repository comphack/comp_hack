/**
 * @file tools/cathedral/src/EventBaseUI.h
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Definition for an event base object.
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

#ifndef TOOLS_CATHEDRAL_SRC_EVENTBASEUI_H
#define TOOLS_CATHEDRAL_SRC_EVENTBASEUI_H

// Qt Includes
#include <PushIgnore.h>
#include <QWidget>
#include <PopIgnore.h>

// objects Includes
#include <EventBase.h>

namespace Ui
{

class EventBase;

} // namespace Ui

class MainWindow;

class EventBase : public QWidget
{
    Q_OBJECT

public:
    explicit EventBase(MainWindow *pMainWindow, QWidget *pParent = 0);
    virtual ~EventBase();

    void Load(const std::shared_ptr<objects::EventBase>& e);
    std::shared_ptr<objects::EventBase> Save() const;

public slots:
    virtual void ToggleBaseDisplay();

protected:
    Ui::EventBase *ui;

    MainWindow *mMainWindow;

    std::shared_ptr<objects::EventBase> mEventBase;
};

#endif // TOOLS_CATHEDRAL_SRC_EVENTBASEUI_H
