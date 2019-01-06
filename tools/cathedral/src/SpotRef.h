/**
 * @file tools/cathedral/src/SpotRef.h
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Definition for a zone spot being referenced.
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

#ifndef TOOLS_CATHEDRAL_SRC_SPOTREF_H
#define TOOLS_CATHEDRAL_SRC_SPOTREF_H

// Qt Includes
#include <PushIgnore.h>
#include <QWidget>
#include <PopIgnore.h>

// libcomp Includes
#include <CString.h>

namespace Ui
{

class SpotRef;

} // namespace Ui

class MainWindow;

class SpotRef : public QWidget
{
    Q_OBJECT

public:
    explicit SpotRef(QWidget *pParent = 0);
    virtual ~SpotRef();

    void SetMainWindow(MainWindow *pMainWindow);

    void SetValue(uint32_t spotID);
    uint32_t GetValue() const;

public slots:
    void Show();

protected:
    Ui::SpotRef *ui;

    MainWindow *mMainWindow;
};

#endif // TOOLS_CATHEDRAL_SRC_SPOTREF_H
