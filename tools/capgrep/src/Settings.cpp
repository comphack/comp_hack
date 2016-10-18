/**
 * @file tools/capgrep/src/Settings.h
 * @ingroup capgrep
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Definition of the class used to change settings for the application.
 *
 * Copyright (C) 2010-2016 COMP_hack Team <compomega@tutanota.com>
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

#include "Settings.h"

#include <PushIgnore.h>
#include <QFile>
#include <QSettings>
#include <PopIgnore.h>

#include <stdint.h>

Settings::Settings(QWidget *p) : QDialog(p)
{
    ui.setupUi(this);

    connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(save()));
    connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(close()));

    setAttribute(Qt::WA_DeleteOnClose);

    QSettings settings;

    ui.packetLimit->setValue(settings.value("packet_limit", 0).toInt());
}

void Settings::save()
{
    QSettings settings;
    settings.setValue("packet_limit", ui.packetLimit->value());

    emit packetLimitChanged(ui.packetLimit->value());

    close();
}
