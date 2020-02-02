/**
 * @file tools/capgrep/src/OpenMulti.h
 * @ingroup capgrep
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Dialog to open multiple capture files at once.
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

#ifndef TOOLS_CAPGREP_SRC_OPENMULTI_H
#define TOOLS_CAPGREP_SRC_OPENMULTI_H

#include <PushIgnore.h>
#include "ui_OpenMulti.h"

#include <QList>
#include <QDialog>
#include <PopIgnore.h>

class QComboBox;
class QPushButton;

class OpenMulti : public QDialog
{
    Q_OBJECT

public:
    OpenMulti(QWidget *parent = 0);

signals:
    void filesReady(const QStringList& files);

protected slots:
    void browse();
    void openFiles();

protected:
    QList<QComboBox*> mEdits;
    QList<QPushButton*> mButtons;

    Ui::OpenMulti ui;
};

#endif // TOOLS_CAPGREP_SRC_OPENMULTI_H
