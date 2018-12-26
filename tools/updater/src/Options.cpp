/**
 * @file tools/updater/src/Options.cpp
 * @ingroup tools
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief GUI for the options.
 *
 * This tool will update the game client.
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

#include "Options.h"

#ifdef Q_OS_WIN32

#include <PushIgnore.h>
#include <QFile>
#include <QMessageBox>
#include <PopIgnore.h>

typedef IDirect3D9* (WINAPI *D3DC9)(UINT);

Options::Options(QWidget *p) : QDialog(p,
    Qt::WindowSystemMenuHint | Qt::WindowTitleHint |
    Qt::WindowCloseButtonHint)
{
    ui.setupUi(this);

    // We are modal and should delete when closed.
    setWindowModality(Qt::WindowModal);
    setAttribute(Qt::WA_DeleteOnClose);

    // Load the DirectX library,
    HMODULE hModule = LoadLibrary("d3d9.dll");

    // Create the D3D9 interface.
    D3DC9 Direct3DCreate9 = (D3DC9)GetProcAddress(hModule,"Direct3DCreate9");
    mD3D9 = Direct3DCreate9(D3D_SDK_VERSION);

    // Get the list of video cards.
    D3DADAPTER_IDENTIFIER9 ident;

    for(UINT i = 0; i < mD3D9->GetAdapterCount(); ++i)
    {
        if(D3D_OK == mD3D9->GetAdapterIdentifier(i, 0, &ident))
        {
            char Description[MAX_DEVICE_IDENTIFIER_STRING + 1];
            memcpy(Description, ident.Description, MAX_DEVICE_IDENTIFIER_STRING);
            Description[MAX_DEVICE_IDENTIFIER_STRING] = 0;

            ui.videoCardCombo->addItem(Description);
        }
    }

    connect(ui.screenPreset, SIGNAL(toggled(bool)),
        this, SLOT(UpdatePresetToggle(bool)));
    connect(ui.screenCustom, SIGNAL(toggled(bool)),
        this, SLOT(UpdateCustomToggle(bool)));
    connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(Save()));

    ui.screenPreset->setChecked(true);

    PopulateAdapterModes();

    ui.screenSizeCombo->setCurrentText(tr("%1 x %2").arg(1024).arg(768));
    ui.chatTextCombo->setCurrentIndex(2);

    if(!mScreenSizes.isEmpty())
    {
        QPair<int, int> size = mScreenSizes.at(0);
        ui.screenX->setValue(size.first);
        ui.screenY->setValue(size.second);
    }

    Load();
}

Options::~Options()
{
}

void Options::UpdatePresetToggle(bool toggled)
{
    ui.screenSizeCombo->setEnabled(toggled);
    ui.screenX->setEnabled(!toggled);
    ui.screenY->setEnabled(!toggled);
}

void Options::UpdateCustomToggle(bool toggled)
{
    UpdatePresetToggle(!toggled);
}

void Options::PopulateAdapterModes()
{
    UINT adapter = (UINT)ui.videoCardCombo->currentIndex();

    ui.screenSizeCombo->clear();

    UINT modeCount = mD3D9->GetAdapterModeCount(adapter, D3DFMT_X8R8G8B8);

    QStringList modes;

    mScreenSizes.clear();

    for(UINT i = 0; i < modeCount; ++i)
    {
        D3DDISPLAYMODE mode;

        if(D3D_OK == mD3D9->EnumAdapterModes(adapter, D3DFMT_X8R8G8B8, i, &mode))
        {
            if(800 > mode.Width || 600 > mode.Height)
            {
                continue;
            }

            QString modeString = tr("%1 x %2").arg(mode.Width).arg(mode.Height);

            if(!modes.contains(modeString))
            {
                mScreenSizes.append(QPair<int, int>(mode.Width, mode.Height));
                modes.append(modeString);
                ui.screenSizeCombo->addItem(modeString);
            }
        }
    }
}

void Options::Load()
{
    QFile file("OutsideOption.txt");

    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return;
    }

    QString adapter;
    int chatFontSizeType = 2;
    int color = 32;
    bool fullScreen = false;
    int resolutionX = 1024;
    int resolutionY = 768;

    while(!file.atEnd())
    {
        QString line = QString::fromLocal8Bit(file.readLine()).trimmed();
        QString key, value;

        int sep = line.indexOf(' ');

        if(0 < sep)
        {
            key = line.left(sep).trimmed();
            value = line.mid(sep + 1).trimmed();
        }

        if("-Adapter" == key)
        {
            adapter = value.replace("\"", QString());
        }
        else if("-ChatFontSizeType" == key)
        {
            chatFontSizeType = value.toInt();
        }
        else if("-Color" == key)
        {
            color = value.toInt();
        }
        else if("-FullScreen" == key)
        {
            fullScreen = "true" == value.toLower();
        }
        else if("-ResolutionX" == key)
        {
            resolutionX = value.toInt();
        }
        else if("-ResolutionY" == key)
        {
            resolutionY = value.toInt();
        }
    }

    if(!adapter.isEmpty())
    {
        ui.videoCardCombo->setCurrentText(adapter);
    }

    bool foundResolution = false;

    typedef QPair<int, int> sizePair;

    foreach(const sizePair& size, mScreenSizes)
    {
        if(size.first == resolutionX && size.second == resolutionY)
        {
            foundResolution = true;
            break;
        }
    }

    ui.screenPreset->setChecked(foundResolution);
    ui.screenCustom->setChecked(!foundResolution);

    ui.screenSizeCombo->setCurrentText(tr("%1 x %2").arg(
        resolutionX).arg(resolutionY));
    ui.screenX->setValue(resolutionX);
    ui.screenY->setValue(resolutionY);

    ui.chatTextCombo->setCurrentIndex(chatFontSizeType);
    ui.modeCombo->setCurrentIndex(fullScreen ? 1 : 0);
}

void Options::Save()
{
    QFile file("OutsideOption.txt");

    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::critical(this, tr("Save Error"),
            tr("Failed to save configuration to OutsideOption.txt!"));

        return;
    }

    int screenX = ui.screenX->value();
    int screenY = ui.screenY->value();

    if(ui.screenPreset->isChecked())
    {
        QPair<int, int> size = mScreenSizes.at(
            ui.screenSizeCombo->currentIndex());
        screenX = size.first;
        screenY = size.second;
    }

    file.write(tr("-Adapter \"%1\"\n"
        "-ChatFontSizeType %2\n"
        "-Color 32\n"
        "-FullScreen %3\n"
        "-ResolutionX %4\n"
        "-ResolutionY %5\n").arg(
        ui.videoCardCombo->currentText()).arg(
        ui.chatTextCombo->currentIndex()).arg(
        ui.modeCombo->currentIndex() ? "true" : "false").arg(
        screenX).arg(screenY).toLocal8Bit());

    close();
}

#endif // Q_OS_WIN32
