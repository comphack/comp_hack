/**
 * @file tools/cathedral/src/SettingsWindow.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for the setting configuration window.
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

#include "SettingsWindow.h"

// Cathedral Includes
#include "EventWindow.h"
#include "MainWindow.h"

// Qt Includes
#include <PushIgnore.h>
#include <QFileDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QSettings>

#include "ui_SettingsWindow.h"
#include <PopIgnore.h>

SettingsWindow::SettingsWindow(MainWindow* pMainWindow, bool initializing,
    QWidget *pParent) : QDialog(pParent), mMainWindow(pMainWindow),
    mInitializing(initializing)
{
    ui = new Ui::SettingsWindow;
    ui->setupUi(this);

    QSettings settings;

    ui->crashDump->setText(settings.value("crashDump").toString());
    ui->datastore->setText(settings.value("datastore").toString());

    connect(ui->crashDumpBrowse, SIGNAL(clicked(bool)), this,
        SLOT(BrowseCrashDump()));
    connect(ui->datastoreBrowse, SIGNAL(clicked(bool)), this,
        SLOT(BrowseDatastore()));

    connect(ui->save, SIGNAL(clicked(bool)), this, SLOT(Save()));
}

SettingsWindow::~SettingsWindow()
{
    delete ui;
}

void SettingsWindow::BrowseCrashDump()
{
    QString qPath = QFileDialog::getSaveFileName(this,
        tr("Specify crash dump file"), "", tr("All files (*)"));
    if(qPath.isEmpty())
    {
        return;
    }

    ui->crashDump->setText(qPath);
}

void SettingsWindow::BrowseDatastore()
{
    QString qPath = QFileDialog::getExistingDirectory(this,
        tr("Load Event XML folder"), mMainWindow->GetDialogDirectory());
    if(qPath.isEmpty())
    {
        return;
    }

    ui->datastore->setText(qPath);
}

void SettingsWindow::Save()
{
    if(ui->datastore->text().isEmpty())
    {
        QMessageBox err;
        err.setText("Please specify a datastore path");
        err.exec();
        return;
    }

    QSettings settings;

    auto oldDatastore = settings.value("datastore").toString();
    if(!QDir(ui->datastore->text()).exists())
    {
        QMessageBox err;
        err.setText("Please select a valid datastore path");
        err.exec();
        return;
    }
    else if(!mInitializing && !oldDatastore.isEmpty() &&
        oldDatastore != ui->datastore->text())
    {
        QMessageBox err;
        err.setText("Please restart the application for the datastore"
            " update to take effect");
        err.exec();
    }

    settings.setValue("crashDump", ui->crashDump->text());
    settings.setValue("datastore", ui->datastore->text());
    settings.sync();

    close();
}
