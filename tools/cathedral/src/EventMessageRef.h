/**
 * @file tools/cathedral/src/EventMessageRef.h
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Definition for an event message being referenced.
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

#ifndef TOOLS_CATHEDRAL_SRC_EVENTMESSAGEREF_H
#define TOOLS_CATHEDRAL_SRC_EVENTMESSAGEREF_H

// Qt Includes
#include <PushIgnore.h>
#include <QWidget>
#include <PopIgnore.h>

namespace Ui
{

class EventMessageRef;

} // namespace Ui

class MainWindow;

class EventMessageRef : public QWidget
{
    Q_OBJECT

public:
    explicit EventMessageRef(QWidget *pParent = 0);
    virtual ~EventMessageRef();

    void SetMainWindow(MainWindow *pMainWindow);

    void SetValue(int32_t value);
    int32_t GetValue() const;

private slots:
    void MessageIDChanged();

protected:
    Ui::EventMessageRef *ui;

    MainWindow *mMainWindow;
};

#endif // TOOLS_CATHEDRAL_SRC_EVENTMESSAGEREF_H
