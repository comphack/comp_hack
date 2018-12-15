/**
 * @file tools/cathedral/src/ObjectPositionUI.h
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Definition for a configured ObjectPosition.
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

#ifndef TOOLS_CATHEDRAL_SRC_OBJECTPOSITIONUI_H
#define TOOLS_CATHEDRAL_SRC_OBJECTPOSITIONUI_H

// Qt Includes
#include <PushIgnore.h>
#include <QWidget>
#include <PopIgnore.h>

// Standard C++11 Includes
#include <memory>

namespace objects
{

class ObjectPosition;

} // namespace objects

namespace Ui
{

class ObjectPosition;

} // namespace Ui

class ObjectPosition : public QWidget
{
    Q_OBJECT

public:
    explicit ObjectPosition(QWidget *pParent = 0);
    virtual ~ObjectPosition();

    void Load(const std::shared_ptr<objects::ObjectPosition>& pos);
    void Load(uint32_t spotID, float x, float y, float rot);
    std::shared_ptr<objects::ObjectPosition> Save() const;

protected slots:
    void RadioToggle();

protected:
    Ui::ObjectPosition *prop;
};

#endif // TOOLS_CATHEDRAL_SRC_OBJECTPOSITIONUI_H
