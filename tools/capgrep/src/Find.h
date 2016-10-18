/**
 * @file tools/capgrep/src/Find.h
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

#ifndef TOOLS_CAPGREP_SRC_FIND_H
#define TOOLS_CAPGREP_SRC_FIND_H

#include <PushIgnore.h>
#include "ui_Find.h"
#include <PopIgnore.h>

class SearchFilter;
class PacketListFilter;

/**
 * Dialog used to find a packet. A packet can be found with three different
 * methods: command code, text, binary. Command codes are input as hex values
 * (like in the filter dialog). Text can be converted to one of multiple
 * encodings selectable from a drop-down list. Once the text is converted, the
 * binary sequence is then searched for. Binary searches convert a hex sequence
 * into a byte encoded sequence to match in the packet data. The results of a
 * search are displayed in a list and the search term for the list is
 * indicated. Double-clicking an item in the list displays the matching
 * packet/command in the main window. Note that the search results have the
 * sample command code filter as the list in the main window.
 */
class Find : public QWidget
{
    Q_OBJECT

public:
    /**
     * Construct the find dialog.
     * @arg model The item model to search.
     * @parent Parent widget (or null if this widget is a window).
     */
    Find(PacketListFilter *model, QWidget *parent = 0);

public slots:
    /**
     * Find the current search term.
     */
    void findTerm();

    /**
     * Find the binary sequence.
     */
    void findTerm(const QByteArray& term);

protected slots:
    /**
     * The 'Cancel' button was clicked, close the window.
     */
    void cancelSearch();

    /**
     * The search type has changed. The search type can be one of:
     * command code, text, binary.
     */
    void termTypeChanged();

    /**
     * An item in the search results has been double-clicked. This method will
     * display the matching data in the main window.
     */
    void doubleClicked(const QModelIndex& index);

protected:
    /// Model to display the search results.
    SearchFilter *mFilter;

    /// Generated UI for the window.
    Ui::Find ui;
};

#endif // TOOLS_CAPGREP_SRC_FIND_H
