/**
 * @file tools/cathedral/src/SpawnLocationUI.h
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Definition for a configured SpawnLocation.
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

#ifndef TOOLS_CATHEDRAL_SRC_SPAWNLOCATIONUI_H
#define TOOLS_CATHEDRAL_SRC_SPAWNLOCATIONUI_H

// Qt Includes
#include <PushIgnore.h>
#include <QWidget>
#include <PopIgnore.h>

// Standard C++11 Includes
#include <memory>

namespace objects
{

class SpawnLocation;

} // namespace objects

namespace Ui
{

class SpawnLocation;

} // namespace Ui

class SpawnLocation : public QWidget
{
    Q_OBJECT

public:
    explicit SpawnLocation(QWidget *pParent = 0);
    virtual ~SpawnLocation();

    void Load(const std::shared_ptr<objects::SpawnLocation>& loc);
    std::shared_ptr<objects::SpawnLocation> Save() const;

protected:
    Ui::SpawnLocation *prop;
};

#endif // TOOLS_CATHEDRAL_SRC_SPAWNLOCATIONUI_H
