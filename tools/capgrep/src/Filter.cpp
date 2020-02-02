/**
 * @file tools/capgrep/src/Filter.cpp
 * @ingroup capgrep
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Packet filter dialog.
 *
 * Copyright (C) 2010-2020 COMP_hack Team <compomega@tutanota.com>
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

#include "Filter.h"
#include "MainWindow.h"

#include <PushIgnore.h>
#include <QInputDialog>
#include <QMessageBox>
#include <PopIgnore.h>

Filter::Filter(QWidget *p) : QDialog(p)
{
    // Setup the dialog UI.
    ui.setupUi(this);

    // Get the filter for the packet list.
    MainWindow *mainWindow = MainWindow::getSingletonPtr();
    PacketListFilter *packetFilter = mainWindow->packetFilter();

    // Set the current white list and black list.
    mWhiteList = packetFilter->white();
    mBlackList = packetFilter->black();

    // Add each item in the white list to the QListWidget.
    foreach(uint16_t cmd, mWhiteList)
        ui.whiteList->addItem(cmdStr(cmd));

    // Add each item in the black list to the QListWidget.
    foreach(uint16_t cmd, mBlackList)
        ui.blackList->addItem(cmdStr(cmd));

    // Connect the 'OK' and 'Cancel' buttons.
    connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(save()));
    connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(cancel()));

    // Connect the 'Add' buttons.
    connect(ui.whiteAdd, SIGNAL(clicked(bool)), this, SLOT(addWhite()));
    connect(ui.blackAdd, SIGNAL(clicked(bool)), this, SLOT(addBlack()));

    // Connect the 'Remove' buttons.
    connect(ui.whiteRemove, SIGNAL(clicked(bool)), this, SLOT(removeWhite()));
    connect(ui.blackRemove, SIGNAL(clicked(bool)), this, SLOT(removeBlack()));

    // Make sure the remove buttons are disabled when the selection becomes
    // empty and re-enabled when an item is selected by calling these methods.
    connect(ui.whiteList, SIGNAL(itemSelectionChanged()),
        this, SLOT(whiteSelectionChanged()));
    connect(ui.blackList, SIGNAL(itemSelectionChanged()),
        this, SLOT(blackSelectionChanged()));

    // Make sure the remove buttons are disabled because no item is selected.
    whiteSelectionChanged();
    blackSelectionChanged();
}

QString Filter::cmdStr(uint16_t cmd)
{
    // Convert the command code to a string.
    return tr("CMD%1").arg(cmd, 4, 16, QLatin1Char('0'));
}

void Filter::save()
{
    // Get the filter for the packet list.
    MainWindow *mainWindow = MainWindow::getSingletonPtr();
    PacketListFilter *packetFilter = mainWindow->packetFilter();

    // Set the white list and the black list for the filter to the new values.
    packetFilter->setFilter(mWhiteList, mBlackList);

    // Close the dialog.
    close();
}

void Filter::cancel()
{
    // Just close the dialog (don't save).
    close();
}

void Filter::addWhite()
{
    // Variable to indicate if the 'OK' button was pressed.
    bool ok = false;

    // Prompt the user to enter a command code to add to the white list.
    QString value = QInputDialog::getText(this, tr("White List Command"),
        tr("Command (hex):"), QLineEdit::Normal, QString(), &ok);
    if(!ok || value.isEmpty())
        return;

    // Convert the string to an integer. This method can handle a "0x" prefix.
    uint16_t cmd = (uint16_t)value.trimmed().toUShort(&ok, 16);

    // Check that the string is a valid integer.
    if(!ok)
    {
        QMessageBox::critical(this, tr("Invalid Command Code"),
            tr("The command code you entered is invalid. A hex value between "
            "0x0000 and 0xFFFF was expected."));

        return;
    }

    // Check if the command code is already in the list.
    if(mWhiteList.contains(cmd))
    {
        QMessageBox::warning(this, tr("Duplicate Command Code"),
            tr("The command code you entered is already in the white list."));

        return;
    }

    // Add the command code to the list.
    mWhiteList.append(cmd);

    // Add the command code to the UI.
    ui.whiteList->addItem(cmdStr(cmd));
}

void Filter::addBlack()
{
    // Variable to indicate if the 'OK' button was pressed.
    bool ok = false;

    // Prompt the user to enter a command code to add to the black list.
    QString value = QInputDialog::getText(this, tr("Black List Command"),
        tr("Command (hex):"), QLineEdit::Normal, QString(), &ok);
    if(!ok || value.isEmpty())
        return;

    // Convert the string to an integer. This method can handle a "0x" prefix.
    uint16_t cmd = (uint16_t)value.trimmed().toUShort(&ok, 16);

    // Check that the string is a valid integer.
    if(!ok)
    {
        QMessageBox::critical(this, tr("Invalid Command Code"),
            tr("The command code you entered is invalid. A hex value between "
            "0x0000 and 0xFFFF was expected."));

        return;
    }

    // Check if the command code is already in the list.
    if(mBlackList.contains(cmd))
    {
        QMessageBox::warning(this, tr("Duplicate Command Code"),
            tr("The command code you entered is already in the black list."));

        return;
    }

    // Add the command code to the list.
    mBlackList.append(cmd);

    // Add the command code to the UI.
    ui.blackList->addItem(cmdStr(cmd));
}

void Filter::removeWhite()
{
    // Sanity check that an item is actually selected.
    if(ui.whiteList->selectedItems().isEmpty())
        return;

    // Get the row of the item selected and use it as the index
    // into the white list.
    int row = ui.whiteList->row(ui.whiteList->selectedItems().first());

    // Sanity check the row (or index).
    if(row < 0)
        return;

    // Remove the command code from the white list at the given index.
    mWhiteList.removeAt(row);

    // Remove the item from the UI and delete it.
    delete ui.whiteList->takeItem(row);
}

void Filter::removeBlack()
{
    // Sanity check that an item is actually selected.
    if(ui.blackList->selectedItems().isEmpty())
        return;

    // Get the row of the item selected and use it as the index
    // into the black list.
    int row = ui.blackList->row(ui.blackList->selectedItems().first());

    // Sanity check the row (or index).
    if(row < 0)
        return;

    // Remove the command code from the black list at the given index.
    mBlackList.removeAt(row);

    // Remove the item from the UI and delete it.
    delete ui.blackList->takeItem(row);
}

void Filter::whiteSelectionChanged()
{
    // Enable the white list remove button if an item is selected in the list.
    ui.whiteRemove->setEnabled(!ui.whiteList->selectedItems().isEmpty());
}

void Filter::blackSelectionChanged()
{
    // Enable the black list remove button if an item is selected in the list.
    ui.blackRemove->setEnabled(!ui.blackList->selectedItems().isEmpty());
}
