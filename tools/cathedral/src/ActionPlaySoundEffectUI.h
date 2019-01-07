/**
 * @file tools/cathedral/src/ActionPlaySoundEffectUI.h
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Definition for a play sound effect action.
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

#ifndef TOOLS_CATHEDRAL_SRC_ACTIONPLAYSOUNDEFFECTUI_H
#define TOOLS_CATHEDRAL_SRC_ACTIONPLAYSOUNDEFFECTUI_H

// Cathedral Includes
#include "ActionUI.h"

// objects Includes
#include <ActionPlaySoundEffect.h>

// Qt Includes
#include <PushIgnore.h>
#include <QWidget>
#include <PopIgnore.h>

namespace Ui
{

class ActionPlaySoundEffect;

} // namespace Ui

class MainWindow;

class ActionPlaySoundEffect : public Action
{
    Q_OBJECT

public:
    explicit ActionPlaySoundEffect(ActionList *pList, MainWindow *pMainWindow,
        QWidget *pParent = 0);
    virtual ~ActionPlaySoundEffect();

    void Load(const std::shared_ptr<objects::Action>& act) override;
    std::shared_ptr<objects::Action> Save() const override;

protected:
    Ui::ActionPlaySoundEffect *prop;

    std::shared_ptr<objects::ActionPlaySoundEffect> mAction;
};

#endif // TOOLS_CATHEDRAL_SRC_ACTIONPLAYSOUNDEFFECTUI_H
