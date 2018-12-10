/**
 * @file tools/cathedral/src/ActionIDs.h
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Definition for a list of IDs (for use in actions).
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

#ifndef TOOLS_CATHEDRAL_SRC_ACTIONIDS_H
#define TOOLS_CATHEDRAL_SRC_ACTIONIDS_H

// Qt Includes
#include <PushIgnore.h>
#include <QWidget>
#include <PopIgnore.h>

// objects Includes
#include <Action.h>

// Standard C++11 Includes
#include <set>

namespace Ui
{

class ActionIDs;

} // namespace Ui

class ActionIDsItem;
class MainWindow;

class ActionIDs : public QWidget
{
    Q_OBJECT

public:
    explicit ActionIDs(QWidget *pParent = 0);
    virtual ~ActionIDs();

    void SetMainWindow(MainWindow *pMainWindow);

    void Load(const std::set<uint32_t>& values);
    std::set<uint32_t> SaveSet() const;

    void Load(const std::list<uint32_t>& values);
    std::list<uint32_t> SaveList() const;

    void RemoveValue(ActionIDsItem *pValue);

protected slots:
    void AddNewValue();

protected:
    void AddValue(ActionIDsItem *pValue);
    void ClearValues();

    Ui::ActionIDs *ui;

    MainWindow *mMainWindow;
    std::list<ActionIDsItem*> mValues;
};

#endif // TOOLS_CATHEDRAL_SRC_ACTIONIDS_H
