/**
 * @file tools/cathedral/src/SpawnList.h
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Definition for a control that holds a list of spawns.
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

#ifndef TOOLS_CATHEDRAL_SRC_SPAWNLIST_H
#define TOOLS_CATHEDRAL_SRC_SPAWNLIST_H

#include "ObjectList.h"

namespace Ui
{

class Spawn;

} // namespace Ui

class SpawnList : public ObjectList
{
    Q_OBJECT

public:
    explicit SpawnList(QWidget *pParent = 0);
    virtual ~SpawnList();

    virtual void SetMainWindow(MainWindow *pMainWindow);

    QString GetObjectID(const std::shared_ptr<
        libcomp::Object>& obj) const override;
    QString GetObjectName(const std::shared_ptr<
        libcomp::Object>& obj) const override;

    void LoadProperties(const std::shared_ptr<libcomp::Object>& obj) override;
    void SaveProperties(const std::shared_ptr<libcomp::Object>& obj) override;

private slots:
    void BaseAITypeToggled(bool checked);
    void UpdateAIDisplay();

protected:
    Ui::Spawn *prop;
};

#endif // TOOLS_CATHEDRAL_SRC_SPAWNLIST_H
