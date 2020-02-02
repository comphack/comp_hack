/**
 * @file tools/updater/src/Options.h
 * @ingroup tools
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief GUI for the options.
 *
 * This tool will update the game client.
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

#ifndef TOOLS_UPDATER_SRC_OPTIONS_H
#define TOOLS_UPDATER_SRC_OPTIONS_H

#include <PushIgnore.h>
#include "ui_Options.h"

#include <QDialog>
#include <PopIgnore.h>

#ifdef Q_OS_WIN32

#include <PushIgnore.h>
#include <d3d9.h>
#include <PopIgnore.h>

class Options : public QDialog
{
    Q_OBJECT

public:
    Options(QWidget *parent = 0);
    ~Options();

protected slots:
    void UpdatePresetToggle(bool toggled);
    void UpdateCustomToggle(bool toggled);

    void Load();
    void Save();

    void LanguageChanged();

protected:
    virtual void changeEvent(QEvent *pEvent);

    void PopulateAdapterModes();

    IDirect3D9 *mD3D9;

    QList< QPair<int, int> > mScreenSizes;

    Ui::Options ui;
};

#endif // Q_OS_WIN32

#endif // TOOLS_UPDATER_SRC_OPTIONS_H
