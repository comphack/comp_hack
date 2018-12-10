/**
 * @file tools/cathedral/src/ActionPlayBGMUI.h
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Definition for a play BGM action.
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

#ifndef TOOLS_CATHEDRAL_SRC_ACTIONPLAYBGMUI_H
#define TOOLS_CATHEDRAL_SRC_ACTIONPLAYBGMUI_H

// Cathedral Includes
#include "ActionUI.h"

// objects Includes
#include <ActionPlayBGM.h>

// Qt Includes
#include <PushIgnore.h>
#include <QWidget>
#include <PopIgnore.h>

namespace Ui
{

class ActionPlayBGM;

} // namespace Ui

class MainWindow;

class ActionPlayBGM : public Action
{
    Q_OBJECT

public:
    explicit ActionPlayBGM(ActionList *pList, MainWindow *pMainWindow,
        QWidget *pParent = 0);
    virtual ~ActionPlayBGM();

    void Load(const std::shared_ptr<objects::Action>& act) override;
    std::shared_ptr<objects::Action> Save() const override;

protected:
    Ui::ActionPlayBGM *prop;

    MainWindow *mMainWindow;

    std::shared_ptr<objects::ActionPlayBGM> mAction;
};

#endif // TOOLS_CATHEDRAL_SRC_ACTIONPLAYBGMUI_H
