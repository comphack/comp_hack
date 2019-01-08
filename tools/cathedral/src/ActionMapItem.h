/**
 * @file tools/cathedral/src/ActionMapItem.h
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Definition for an item in an action map.
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

#ifndef TOOLS_CATHEDRAL_SRC_ACTIONMAPITEM_H
#define TOOLS_CATHEDRAL_SRC_ACTIONMAPITEM_H

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

class ActionMapItem;

} // namespace Ui

class ActionMap;
class MainWindow;

class ActionMapItem : public QWidget
{
    Q_OBJECT

public:
    explicit ActionMapItem(const QString& valueName, ActionMap *pMap,
        QWidget *pParent = 0);
    virtual ~ActionMapItem();

    void Setup(int32_t key, int32_t value,
        const libcomp::String& objectSelectorType,
        bool selectorServerData, MainWindow* pMainWindow);

    int32_t GetKey() const;
    int32_t GetValue() const;

    void SetMinMax(int32_t min, int32_t max);

public slots:
    void Remove();

protected:
    Ui::ActionMapItem *ui;

    ActionMap *mMap;
};

#endif // TOOLS_CATHEDRAL_SRC_ACTIONMAPITEM_H
