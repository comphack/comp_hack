/**
 * @file tools/cathedral/src/ActionAddRemoveItemsUI.h
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Definition for an add/remove items action.
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

#ifndef TOOLS_CATHEDRAL_SRC_ACTIONADDREMOVEITEMSUI_H
#define TOOLS_CATHEDRAL_SRC_ACTIONADDREMOVEITEMSUI_H

// Cathedral Includes
#include "ActionUI.h"

// objects Includes
#include <ActionAddRemoveItems.h>

// Qt Includes
#include <PushIgnore.h>
#include <QWidget>
#include <PopIgnore.h>

namespace Ui
{

class ActionAddRemoveItems;

} // namespace Ui

class MainWindow;

class ActionAddRemoveItems : public Action
{
    Q_OBJECT

public:
    explicit ActionAddRemoveItems(ActionList *pList, MainWindow *pMainWindow,
        QWidget *pParent = 0);
    virtual ~ActionAddRemoveItems();

    void Load(const std::shared_ptr<objects::Action>& act) override;
    std::shared_ptr<objects::Action> Save() const override;

protected slots:
    void ModeChanged();

protected:
    Ui::ActionAddRemoveItems *prop;

    std::shared_ptr<objects::ActionAddRemoveItems> mAction;
};

#endif // TOOLS_CATHEDRAL_SRC_ACTIONADDREMOVEITEMSUI_H
