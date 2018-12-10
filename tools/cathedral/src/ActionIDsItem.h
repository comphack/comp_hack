/**
 * @file tools/cathedral/src/ActionIDsItem.h
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Definition for an item in an action IDs list.
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

#ifndef TOOLS_CATHEDRAL_SRC_ACTIONIDSITEM_H
#define TOOLS_CATHEDRAL_SRC_ACTIONIDSITEM_H

// Qt Includes
#include <PushIgnore.h>
#include <QWidget>
#include <PopIgnore.h>

// objects Includes
#include <Action.h>

// Standard C++11 Includes
#include <map>

namespace Ui
{

class ActionIDsItem;

} // namespace Ui

class ActionIDs;
class MainWindow;

class ActionIDsItem : public QWidget
{
    Q_OBJECT

public:
    explicit ActionIDsItem(ActionIDs *pIDs, uint32_t value,
        QWidget *pParent = 0);
    explicit ActionIDsItem(ActionIDs *pIDs, QWidget *pParent = 0);
    virtual ~ActionIDsItem();

    uint32_t GetValue() const;

public slots:
    void Remove();

protected:
    Ui::ActionIDsItem *ui;

    ActionIDs *mIDs;
};

#endif // TOOLS_CATHEDRAL_SRC_ACTIONIDSITEM_H
