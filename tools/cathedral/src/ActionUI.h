/**
 * @file tools/cathedral/src/ActionUI.h
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Definition for an action.
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

#ifndef TOOLS_CATHEDRAL_SRC_ACTIONUI_H
#define TOOLS_CATHEDRAL_SRC_ACTIONUI_H

// Qt Includes
#include <PushIgnore.h>
#include <QWidget>
#include <PopIgnore.h>

// objects Includes
#include <Action.h>

namespace Ui
{

class Action;

} // namespace Ui

class ActionList;
class MainWindow;

class Action : public QWidget
{
    Q_OBJECT

public:
    explicit Action(ActionList *pList, MainWindow *pMainWindow,
        QWidget *pParent = 0);
    virtual ~Action();

    virtual void Load(const std::shared_ptr<objects::Action>& act) = 0;
    virtual std::shared_ptr<objects::Action> Save() const = 0;

    virtual void UpdatePosition(bool isFirst, bool isLast);

public slots:
    virtual void Remove();
    virtual void MoveUp();
    virtual void MoveDown();
    virtual void ToggleBaseDisplay();

protected:
    void LoadBaseProperties(const std::shared_ptr<objects::Action>& action);
    void SaveBaseProperties(const std::shared_ptr<objects::Action>& action) const;

    Ui::Action *ui;

    ActionList *mList;
    MainWindow *mMainWindow;
};

#endif // TOOLS_CATHEDRAL_SRC_ACTIONUI_H
