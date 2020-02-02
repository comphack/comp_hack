/**
 * @file tools/cathedral/src/FindRefWindow.h
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Definition for a window that finds reference object uses.
 *
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
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

#ifndef TOOLS_CATHEDRAL_SRC_FINDREFWINDOW_H
#define TOOLS_CATHEDRAL_SRC_FINDREFWINDOW_H

// Standard C++11 Includes
#include <memory>

// libcomp Includes
#include <CString.h>
#include <EnumMap.h>

// objects Includes
#include <Action.h>
#include <Event.h>
#include <EventCondition.h>

// Qt Includes
#include <PushIgnore.h>
#include "ui_FindRefWindow.h"
#include <PopIgnore.h>

namespace objects
{

class DropSet;
class ServerZone;
class ServerZonePartial;
class Spawn;

} // namespace objects

class MainWindow;

class FindRefWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit FindRefWindow(MainWindow *pMainWindow, QWidget *pParent = 0);
    virtual ~FindRefWindow();

    bool Open(const libcomp::String& objType, uint32_t val);

    void closeEvent(QCloseEvent* event);

private slots:
    void Export();
    void Find();

    void SetZoneDirectory();
    void ToggleZoneDirectory();

private:
    void BuildDropSetFilters();
    void BuildCEventMessageDataFilters();
    void BuildCHouraiDataFilters();
    void BuildCHouraiMessageDataFilters();
    void BuildCItemDataFilters();
    void BuildCKeyItemDataFilters();
    void BuildCQuestDataFilters();
    void BuildCSoundDataFilters();
    void BuildCTitleDataFilters();
    void BuildCValuablesDataFilters();
    void BuildDevilDataFilters();
    void BuildHNPCDataFilters();
    void BuildONPCDataFilters();
    void BuildShopProductDataFilters();
    void BuildStatusDataFilters();
    void BuildZoneDataFilters();

    void GetValue1(const std::shared_ptr<objects::EventCondition>& c,
        std::set<uint32_t>& ids);

    void FindAsync();

    void AddResult(uint32_t id, const libcomp::String& location,
        const libcomp::String& section);

    void FilterActionIDs(
        const std::list<std::shared_ptr<objects::Action>>& actions,
        std::set<uint32_t>& ids);

    std::set<uint32_t> GetFilteredIDs(const std::set<uint32_t>& ids,
        uint32_t value, uint32_t maxValue);

    MainWindow *mMainWindow;

    libcomp::EnumMap<objects::Action::ActionType_t,
        std::function<void(const std::shared_ptr<objects::Action>&,
            std::set<uint32_t>&)>> mActionFilters;

    libcomp::EnumMap<objects::Event::EventType_t,
        std::function<void(const std::shared_ptr<objects::Event>&,
            std::set<uint32_t>&)>> mEventFilters;

    libcomp::EnumMap<objects::EventCondition::Type_t,
        std::function<void(FindRefWindow&,
            const std::shared_ptr<objects::EventCondition>&,
            std::set<uint32_t>&)>> mEventConditionFilters;

    std::function<void(const std::shared_ptr<objects::DropSet>&,
        std::set<uint32_t>&)> mDropSetFilter;

    std::function<void(const std::shared_ptr<objects::Spawn>&,
        std::set<uint32_t>&)> mSpawnFilter;

    std::function<void(const std::shared_ptr<objects::ServerZone>&,
        std::set<uint32_t>&)> mZoneFilter;

    std::function<void(const std::shared_ptr<objects::ServerZonePartial>&,
        std::set<uint32_t>&)> mZonePartialFilter;

    libcomp::String mObjType;

    Ui::FindRefWindow *ui;
};

#endif // TOOLS_CATHEDRAL_SRC_FINDREFWINDOW_H
