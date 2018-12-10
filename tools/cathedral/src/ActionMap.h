/**
 * @file tools/cathedral/src/ActionMap.h
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Definition for a map of IDs to a value (for use in actions).
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

#ifndef TOOLS_CATHEDRAL_SRC_ACTIONMAP_H
#define TOOLS_CATHEDRAL_SRC_ACTIONMAP_H

// Qt Includes
#include <PushIgnore.h>
#include <QWidget>
#include <PopIgnore.h>

// objects Includes
#include <Action.h>

// Standard C++11 Includes
#include <unordered_map>

namespace Ui
{

class ActionMap;

} // namespace Ui

class ActionMapItem;
class MainWindow;

class ActionMap : public QWidget
{
    Q_OBJECT

public:
    explicit ActionMap(QWidget *pParent = 0);
    virtual ~ActionMap();

    void SetMainWindow(MainWindow *pMainWindow);

    void Load(const std::unordered_map<uint32_t, int32_t>& values);
    std::unordered_map<uint32_t, int32_t> Save() const;

    void RemoveValue(ActionMapItem *pValue);
    void SetValueName(const QString& name);
    void SetMinMax(int32_t min, int32_t max);

protected slots:
    void AddNewValue();

protected:
    void AddValue(ActionMapItem *pValue);
    void ClearValues();

    QString mValueName;
    int32_t mMin, mMax;

    Ui::ActionMap *ui;

    MainWindow *mMainWindow;
    std::list<ActionMapItem*> mValues;
};

#endif // TOOLS_CATHEDRAL_SRC_ACTIONMAP_H
