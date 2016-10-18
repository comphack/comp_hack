/**
 * @file tools/capgrep/src/Find.cpp
 * @ingroup capgrep
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Packet find dialog.
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

#include "Find.h"
#include "MainWindow.h"
#include "SearchFilter.h"

// Includes from libcomp.
#include <Convert.h>

#include <PushIgnore.h>
#include <QList>
#include <QRegExp>
#include <QSettings>

#include <QMessageBox>
#include <PopIgnore.h>

#include <stdint.h>

Find::Find(PacketListFilter *model, QWidget *p) : QWidget(p)
{
    // Populate the user interface from the generated code.
    ui.setupUi(this);

    // Hide the label until a search is made.
    ui.searchLabel->hide();

    QSettings settings;

    // Set the default encoding to UTF-8.
    int encoding = 2; // UTF-8

    // Get the last used encoding. This encoding is used by the main window
    // to display text strings in the packets.
    QString encStr = settings.value("encoding", "utf8").toString().toLower();

    // Convert the string to a numeric index for use with the combo box.
    if(encStr == "cp1252")
        encoding = 0;
    else if(encStr == "cp932")
        encoding = 1;

    // Create the search filter model and set the source model (the filter
    // model for the main packet list).
    mFilter = new SearchFilter;
    mFilter->setSourceModel(model);

    // Set the encoding for text searches.
    ui.encoding->setCurrentIndex(encoding);

    // Set the model for the search results list.
    ui.findList->setModel(mFilter);

    // Get the last used search type.
    QString searchType = settings.value("search_type",
        "binary").toString().toLower();

    // Select the last used search type.
    if(searchType == "text")
        ui.textButton->setChecked(true);
    else if(searchType == "command")
        ui.commandButton->setChecked(true);
    else // "binary"
        ui.binaryButton->setChecked(true);

    // Connect all signals to the appropriate slot method.
    connect(ui.findEdit, SIGNAL(returnPressed()), this, SLOT(findTerm()));
    connect(ui.findButton, SIGNAL(clicked(bool)), this, SLOT(findTerm()));
    connect(ui.cancelButton, SIGNAL(clicked(bool)),
        this, SLOT(cancelSearch()));
    connect(ui.findList, SIGNAL(doubleClicked(const QModelIndex&)),
        this, SLOT(doubleClicked(const QModelIndex&)));
    connect(ui.textButton, SIGNAL(toggled(bool)),
        this, SLOT(termTypeChanged()));
    connect(ui.binaryButton, SIGNAL(toggled(bool)),
        this, SLOT(termTypeChanged()));
    connect(ui.commandButton, SIGNAL(toggled(bool)),
        this, SLOT(termTypeChanged()));

    // Update the UI for the search type.
    termTypeChanged();
}

void Find::termTypeChanged()
{
    QSettings settings;

    // Save the current search type as the last type used.
    if(ui.binaryButton->isChecked())
        settings.setValue("search_type", "binary");
    else if(ui.textButton->isChecked())
        settings.setValue("search_type", "text");
    else // ui.commandButton->isChecked()
        settings.setValue("search_type", "command");

    // Determine if the search type is text.
    bool isText = ui.textButton->isChecked();

    // If the search type is text, display the encoding combo box (and it's
    // associated label); otherwise, hide them.
    ui.encodingLabel->setVisible(isText);
    ui.encoding->setVisible(isText);
}

void Find::findTerm()
{
    // Get the search term entered.
    QString term = ui.findEdit->text();

    // Make sure there is a search term.
    if(term.isEmpty())
    {
        // Tell the user how stupid they are.
        QMessageBox::critical(this, tr("Find Error"),
            tr("You must enter a search term."));

        return;
    }

    // Perform a different search based on the search type.
    if(ui.binaryButton->isChecked())
    {
        // Remove spaces between the hex digits.
        term.replace(" ", QString());

        // Make sure the term isn't empty now.
        if(term.isEmpty())
        {
            // Tell the user how stupid they are.
            QMessageBox::critical(this, tr("Find Error"),
                tr("You must enter a search term."));

            return;
        }

        // Since we are searching for complete bytes and each byte is two hex
        // digits, the search term should have an even length. The search term
        // should only contain valid hex values. We assume you are not going to
        // write the hex prefix before any values (since we are not assuming
        // any endianness and it's silly to add it for every byte).
        if(term.length() % 2 != 0 || !QRegExp("[a-fA-F0-9]+").exactMatch(term))
        {
            QMessageBox::critical(this, tr("Find Error"), tr("A binary search "
                "term must consist solely of a series of hex digit pairs."));

            return;
        }

        QStringList labelText;
        QByteArray binaryTerm;
        uint8_t byte;

        // Convert each byte in the string into an integer. We can assume it
        // will convert right since we checked for valid hex values above.
        while(!term.isEmpty())
        {
            // Convert the byte.
            byte = (uint8_t)term.left(2).toUInt(0, 16);

            // Remove the byte from the search term.
            term = term.mid(2);

            // Add the byte to the binary seqeunce.
            binaryTerm.append((char*)&byte, 1);

            // Add the byte to the search term label.
            labelText.append(tr("%1").arg(
                (uint32_t)byte & 0xFF, 2, 16, QLatin1Char('0')).toUpper());
        }

        // Finish and set the search term label.
        ui.searchLabel->setText(tr("Binary: %1").arg(labelText.join(" ")));

        // Find the binary sequence.
        mFilter->findBinary(binaryTerm);
    }
    else if(ui.textButton->isChecked())
    {
        // Find the string using the desired encoding.
        switch(ui.encoding->currentIndex())
        {
            case 0:
                ui.searchLabel->setText(tr("Text (CP1252): %1").arg(term));

                mFilter->findText("CP1252", term);
                break;
            case 1:
                ui.searchLabel->setText(tr("Text (CP932): %1").arg(term));

                mFilter->findText("CP932", term);
                break;
            case 2:
            default:
                ui.searchLabel->setText(tr("Text (UTF-8): %1").arg(term));

                mFilter->findText("UTF-8", term);
                break;
        }
    }
    else // ui.commandButton->isChecked()
    {
        bool ok = false;

        // Convert the command code to an integer.
        uint16_t cmd = (uint16_t)term.toUShort(&ok, 16);

        // Check if the command code is a valid integer.
        if(!ok)
        {
            QMessageBox::critical(this, tr("Find Error"), tr("A command code "
                "must have a hex value in the range 0x0000-0xFFFF."));

            return;
        }

        // Update the search label to display the command code.
        ui.searchLabel->setText(tr("Command: 0x%1").arg(tr("%1").arg(
            cmd, 4, 16, QLatin1Char('0')).toUpper()));

        // Find all commands with the desired command code.
        mFilter->findCommand(cmd);
    }

    // Make sure the search label is visible. This label shows the
    // search term used.
    ui.searchLabel->show();
}

void Find::findTerm(const QByteArray& term)
{
    QStringList labelText;

    for(int i = 0; i < term.size(); i++)
    {
        // Add the byte to the search term label.
        labelText.append(tr("%1").arg(
            (uint32_t)term.at(i) & 0xFF, 2, 16, QLatin1Char('0')).toUpper());
    }

    // Finish and set the search term label. Make sure the search label is
    // visible. This label shows the search term used.
    ui.searchLabel->setText(tr("Binary: %1").arg(labelText.join(" ")));
    ui.searchLabel->show();

    // Find the binary sequence.
    mFilter->findBinary(term);
}

void Find::doubleClicked(const QModelIndex& index)
{
    int packet = 0;
    int offset = -1;
    QByteArray term;

    // Read the search result that was double-clicked.
    if(!mFilter->searchResult(index, packet, offset, term))
        return;

    // Show the packet in the main window. If a binary sequence (or text) was
    // matched, the section of the packet will be selected.
    MainWindow::getSingletonPtr()->showSelection(packet,
        offset, offset + term.size() - 1);
}

void Find::cancelSearch()
{
    // Clear the search box.
    ui.findEdit->clear();

    // Hide the label.
    ui.searchLabel->hide();

    // Reset the filter to show nothing.
    mFilter->reset();

    // Close the dialog.
    close();
}
