/**
 * @file tools/cathedral/src/ActionRunScriptUI.h
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Definition for a run script action.
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

#ifndef TOOLS_CATHEDRAL_SRC_ACTIONRUNSCRIPTUI_H
#define TOOLS_CATHEDRAL_SRC_ACTIONRUNSCRIPTUI_H

// Cathedral Includes
#include "ActionUI.h"

// objects Includes
#include <ActionRunScript.h>

// Qt Includes
#include <PushIgnore.h>
#include <QWidget>
#include <PopIgnore.h>

namespace Ui
{

class ActionRunScript;

} // namespace Ui

class MainWindow;

class ActionRunScript : public Action
{
    Q_OBJECT

public:
    explicit ActionRunScript(ActionList *pList, MainWindow *pMainWindow,
        QWidget *pParent = 0);
    virtual ~ActionRunScript();

    void Load(const std::shared_ptr<objects::Action>& act) override;
    std::shared_ptr<objects::Action> Save() const override;

protected:
    Ui::ActionRunScript *prop;

    std::shared_ptr<objects::ActionRunScript> mAction;
};

#endif // TOOLS_CATHEDRAL_SRC_ACTIONRUNSCRIPTUI_H
