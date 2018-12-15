/**
 * @file tools/cathedral/src/ServerScript.h
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Definition for a server script usage with optional parameters.
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

#ifndef TOOLS_CATHEDRAL_SRC_SERVERSCRIPT_H
#define TOOLS_CATHEDRAL_SRC_SERVERSCRIPT_H

// Qt Includes
#include <PushIgnore.h>
#include <QWidget>
#include <PopIgnore.h>

// libcomp Includes
#include <CString.h>

// Standard C++11 Includes
#include <list>

namespace Ui
{

class ServerScript;

} // namespace Ui

class ServerScript : public QWidget
{
    Q_OBJECT

public:
    explicit ServerScript(QWidget *pParent = 0);
    virtual ~ServerScript();

    void SetScriptID(const libcomp::String& scriptID);
    libcomp::String GetScriptID() const;

    void SetParams(std::list<libcomp::String>& params);
    std::list<libcomp::String> GetParams() const;

protected:
    Ui::ServerScript *ui;
};

#endif // TOOLS_CATHEDRAL_SRC_SERVERSCRIPT_H
