/**
 * @file tools/logger/src/Settings.cpp
 * @ingroup logger
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Definition of the GUI class to modify the logger settings.
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

#include "LoggerServer.h"
#include "MainWindow.h"

#include <PushIgnore.h>
#include <QDir>
#include <QSettings>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <PopIgnore.h>

Settings::Settings(LoggerServer *server,
    QWidget *p) : QDialog(p), mServer(server)
{
    // Create the GUI and connect all the signals.
    ui.setupUi(this);

    connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(saveAndClose()));
    connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(deleteLater()));

    connect(ui.addClientButton, SIGNAL(clicked(bool)),
        this, SLOT(addClient()));
    connect(ui.removeClientButton, SIGNAL(clicked(bool)),
        this, SLOT(removeClient()));
    connect(ui.clientList, SIGNAL(itemSelectionChanged()),
        this, SLOT(selectedClient()));

    // Load the current settings into the GUI.
    ui.usVersion->setValue(static_cast<int32_t>(server->usVersion()) - 1000);
    ui.jpVersion->setValue(static_cast<int32_t>(server->jpVersion()) - 1000);

    ui.usAddress->setText(server->usAddress());
    ui.jpAddress->setText(server->jpAddress());

    ui.jpWebAuth->setText(server->jpWebAuth());
    ui.jpWebAuthEnabled->setChecked(server->isWebAuthJPEnabled());

    ui.lobbyCheckbox->setChecked(server->isLobbyLogEnabled());
    ui.channelCheckbox->setChecked(server->isChannelLogEnabled());

    ui.closeWarning->setChecked(!QSettings().value(
        "noexitwarning", false).toBool());

    // Load the client list into the list widget.
    QVariantMap clientList = QSettings().value("clientList").toMap();
    QMapIterator<QString, QVariant> it(clientList);

    while( it.hasNext() )
    {
        it.next();

        QString title = it.key();
        QString path = it.value().toString();

        QListWidgetItem *item = new QListWidgetItem(
            tr("%1 (%2)").arg(title).arg(path) );
        item->setData(Qt::UserRole, path);
        item->setData(Qt::AccessibleTextRole, title);

        ui.clientList->addItem(item);
    }
}

void Settings::saveAndClose()
{
    // Save all the settings from the GUI.
    mServer->setVersionUS(static_cast<uint32_t>(ui.usVersion->value()) + 1000u);
    mServer->setVersionJP(static_cast<uint32_t>(ui.jpVersion->value()) + 1000u);

    mServer->setAddressUS(ui.usAddress->text().trimmed());
    mServer->setAddressJP(ui.jpAddress->text().trimmed());

    mServer->setWebAuthJP(ui.jpWebAuth->text().trimmed());
    mServer->setWebAuthJPEnabled(ui.jpWebAuthEnabled->isChecked());

    mServer->setLobbyLogEnabled(ui.lobbyCheckbox->isChecked());
    mServer->setChannelLogEnabled(ui.channelCheckbox->isChecked());

    // Recreate the client list.
    QVariantMap clientList;

    for(int i = 0; i < ui.clientList->count(); i++)
    {
        QListWidgetItem *item = ui.clientList->item(i);

        QString title = item->data(Qt::AccessibleTextRole).toString();
        QString path = item->data(Qt::UserRole).toString();

        clientList[title] = path;
    }

    QSettings settings;
    settings.setValue("clientList", clientList);
    settings.setValue("noexitwarning", !ui.closeWarning->isChecked());

    // Emit the client list as a signal so the main window will update the
    // start game menu.
    emit clientListChanged(clientList);

    // Close and delete the settings window.
    deleteLater();
}

void Settings::addClient()
{
    // Tell the user what to do with the file dialog.
    QMessageBox::information(this, "Locate Client Install",
        "You will now be asked to locate the directory that contains your "
        "client install (the directory that contains the file "
        "ImagineClient.exe).");

    // Present the user with the file dialog to select the client install
    // directory.
    QString path = QDir::toNativeSeparators(
        QFileDialog::getExistingDirectory(this,
        "Locate Client Install",
        "C:\\AeriaGames\\MegaTen") );
    if( path.isEmpty() )
        return;

    float ver;
    bool isUS;

    // Get the version of the client.
    if( !MainWindow::versionCheck(tr("%1/ImagineClient.exe").arg(
        path), &ver, 0, &isUS) )
    {
        QMessageBox::critical(this, tr("Invalid Client"),
            tr("Failed to detect the client version."));

        return;
    }

    char buffer[256];
    memset(buffer, 0, 256);

    if(isUS)
        sprintf(buffer, "IMAGINE Version %4.3fU", ver);
    else
        sprintf(buffer, "IMAGINE Version %4.3f", ver);

    bool ok = false;

    // Prompt the user for the name of the client (default to the version or
    // window title of the client).
    QString title = QInputDialog::getText(this, "Client Name",
        "Plase name this client version:", QLineEdit::Normal,
        QString::fromUtf8(buffer), &ok);

    // If the user didn't input a name or clicked cancel, abort.
    if(title.isEmpty() || !ok)
        return;

    // Add the client to the list.
    QListWidgetItem *item = new QListWidgetItem(
        tr("%1 (%2)").arg(title).arg(path) );
    item->setData(Qt::UserRole, path);
    item->setData(Qt::AccessibleTextRole, title);

    ui.clientList->addItem(item);
}

void Settings::removeClient()
{
    // Get the selected item and remove it from the list.
    QList<QListWidgetItem*> items = ui.clientList->selectedItems();
    if( items.isEmpty() )
        return;

    ui.clientList->removeItemWidget( items.first() );

    delete items.first();
}

void Settings::selectedClient()
{
    // If a client is selected, enable to remove button; otherwise, disable it.
    ui.removeClientButton->setEnabled(
        !ui.clientList->selectedItems().isEmpty() );
}
