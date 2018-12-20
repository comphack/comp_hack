/**
 * @file tools/cathedral/src/ServerScript.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for a server script usage with optional parameters.
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

#include "ServerScript.h"

// Cathedral Includes
#include "MainWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include <QLineEdit>

#include "ui_ServerScript.h"
#include <PopIgnore.h>

ServerScript::ServerScript(QWidget *pParent) : QWidget(pParent)
{
    ui = new Ui::ServerScript;
    ui->setupUi(this);

    ui->params->Setup(DynamicItemType_t::PRIMITIVE_STRING, nullptr);
}

ServerScript::~ServerScript()
{
    delete ui;
}

void ServerScript::SetScriptID(const libcomp::String& scriptID)
{
    ui->scriptID->lineEdit()->setText(qs(scriptID));
}

libcomp::String ServerScript::GetScriptID() const
{
    return libcomp::String(ui->scriptID->currentText().toStdString());
}

void ServerScript::SetParams(std::list<libcomp::String>& params)
{
    for(auto param : params)
    {
        ui->params->AddString(param);
    }
}

std::list<libcomp::String> ServerScript::GetParams() const
{
    return ui->params->GetStringList();
}
