/**
 * @file tools/cathedral/src/ObjectSelector.h
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Definition for a value bound to an object with a selectable
 *  text representation.
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

#ifndef TOOLS_CATHEDRAL_SRC_OBJECTSELECTOR_H
#define TOOLS_CATHEDRAL_SRC_OBJECTSELECTOR_H

#include <ObjectSelectorBase.h>

namespace Ui
{

class ObjectSelector;

} // namespace Ui

class ObjectSelector : public ObjectSelectorBase
{
    Q_OBJECT

public:
    explicit ObjectSelector(QWidget *pParent = 0);
    virtual ~ObjectSelector();

    bool BindSelector(MainWindow *pMainWindow,
        const libcomp::String& objType, bool serverData = false);

    void SetValue(uint32_t value) override;
    uint32_t GetValue() const override;

private slots:
    void ValueChanged();

protected:
    Ui::ObjectSelector *ui;

    bool mServerData;
};

#endif // TOOLS_CATHEDRAL_SRC_OBJECTSELECTOR_H
