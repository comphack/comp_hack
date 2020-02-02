/**
 * @file tools/capgrep/src/Filter.h
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

#ifndef TOOLS_CAPGREP_SRC_FILTER_H
#define TOOLS_CAPGREP_SRC_FILTER_H

#include <PushIgnore.h>
#include "ui_Filter.h"

#include <QDialog>
#include <PopIgnore.h>

#include <stdint.h>

/**
 * Packet filter dialog. The filter dialog presents two lists of packets that
 * will be filtered. If the white list is not empty, only packets in the white
 * list will be displayed. If a packet is in the black list, it will not be
 * displayed.
 */
class Filter : public QDialog
{
    Q_OBJECT

public:
    /**
     * Construct the filter dialog.
     * @parent Parent window (or null if this dialog isn't modal).
     */
    Filter(QWidget *parent = 0);

    /**
     * Convert the given command code to a string representation.
     * @arg cmd Command code to convert to a string.
     * @return String representation of the command code.
     */
    static QString cmdStr(uint16_t cmd);

protected slots:
    /**
     * Save the settings and close the dialog.
     */
    void save();

    /**
     * Discard any changes and close the dialog.
     */
    void cancel();

    /**
     * Prompt the user for a command code to add to the white list.
     */
    void addWhite();

    /**
     * Prompt the user for a command code to add to the black list.
     */
    void addBlack();

    /**
     * Remove the selected command from the white list.
     */
    void removeWhite();

    /**
     * Remove the selected command from the black list.
     */
    void removeBlack();

    /**
     * The white list selection has changed. This is used to check if the
     * remove button should be enabled.
     */
    void whiteSelectionChanged();

    /**
     * The black list selection has changed. This is used to check if the
     * remove button should be enabled.
     */
    void blackSelectionChanged();

protected:
    /// List of command codes in the white list.
    QList<uint16_t> mWhiteList;

    /// List of command codes in the black list.
    QList<uint16_t> mBlackList;

    /// Generated UI for the dialog.
    Ui::Filter ui;
};

#endif // TOOLS_CAPGREP_SRC_FILTER_H
