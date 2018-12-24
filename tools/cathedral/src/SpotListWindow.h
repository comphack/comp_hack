/**
 * @file tools/cathedral/src/SpotListWindow.h
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Definition for a window that holds a list of zone spots.
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

#ifndef TOOLS_CATHEDRAL_SRC_SPOTLISTWINDOW_H
#define TOOLS_CATHEDRAL_SRC_SPOTLISTWINDOW_H

#include "ObjectListWindow.h"

namespace Ui
{

class SpotProperties;

} // namespace Ui

class SpotListWindow : public ObjectListWindow
{
    Q_OBJECT

public:
    explicit SpotListWindow(QWidget *pParent = 0);
    virtual ~SpotListWindow();

    virtual void SetMainWindow(MainWindow *pMainWindow);

    QString GetObjectID(const std::shared_ptr<
        libcomp::Object>& obj) const override;

    void LoadProperties(const std::shared_ptr<libcomp::Object>& obj) override;
    void SaveProperties(const std::shared_ptr<libcomp::Object>& obj) override;

protected:
    Ui::SpotProperties *prop;
};

#endif // TOOLS_CATHEDRAL_SRC_SPOTLISTWINDOW_H
