/**
 * @file tools/cathedral/src/ObjectSelectorWindow.h
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Definition for a value selection window bound to an ObjectSelector.
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

#ifndef TOOLS_CATHEDRAL_SRC_OBJECTSELECTORWINDOW_H
#define TOOLS_CATHEDRAL_SRC_OBJECTSELECTORWINDOW_H

// Qt Includes
#include <PushIgnore.h>
#include <QWidget>
#include <PopIgnore.h>

// libcomp Includes
#include <CString.h>
#include <Object.h>

namespace Ui
{

class ObjectSelectorWindow;

} // namespace Ui

class ObjectSelectorBase;
class ObjectSelectorList;

class ObjectSelectorWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ObjectSelectorWindow(QWidget *pParent = 0);
    virtual ~ObjectSelectorWindow();

    void Bind(ObjectSelectorList* listControl);

    void Open(ObjectSelectorBase* ctrl = 0);

protected slots:
    void ObjectSelected();
    void SelectedObjectChanged();

private:
    Ui::ObjectSelectorWindow *ui;

    ObjectSelectorBase* mSelectorControl;
};

#endif // TOOLS_CATHEDRAL_SRC_OBJECTSELECTORWINDOW_H
