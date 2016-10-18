/**
 * @file tools/capgrep/src/OpenMulti.cpp
 * @ingroup capgrep
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Dialog to open multiple capture files at once.
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

#include "OpenMulti.h"

#include <PushIgnore.h>
#include <QSettings>

#include <QComboBox>
#include <QLineEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>
#include <PopIgnore.h>

OpenMulti::OpenMulti(QWidget *p) : QDialog(p)
{
    ui.setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);

    QSettings settings;
    QStringList recentFiles = settings.value("recentFiles").toStringList();

    mEdits << ui.pathA << ui.pathB << ui.pathC
        << ui.pathD << ui.pathE << ui.pathF;

    mButtons << ui.browseA << ui.browseB << ui.browseC
        << ui.browseD << ui.browseE << ui.browseF;

    for(int i = 0; i < 6; i++)
    {
        QComboBox *edit = mEdits.at(i);

        foreach(QString file, recentFiles)
            edit->addItem(file);

        edit->lineEdit()->clear();

        connect(mButtons.at(i), SIGNAL(clicked(bool)), this, SLOT(browse()));
    }

    connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(openFiles()));
    connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(close()));
}

void OpenMulti::openFiles()
{
    QStringList files;

    int count = 0;

    foreach(QComboBox *edit, mEdits)
    {
        QString file = edit->lineEdit()->text();
        if( !file.isEmpty() )
            count++;

        files << file;
    }

    if(count < 2)
    {
        QMessageBox::warning(this, tr("Open Multiple Failed"),
            tr("You must open at least 2 cpature files."));

        return;
    }

    emit filesReady(files);

    deleteLater();
}

void OpenMulti::browse()
{
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if(!button)
        return;

    if( !mButtons.contains(button) )
        return;

    QComboBox *edit = mEdits.at( mButtons.indexOf(button) );

    QString path = QFileDialog::getOpenFileName(this, tr("Open Capture File"),
        QString(), tr("COMP_hack Channel Capture (*.hack)"));

    if( path.isEmpty() )
        return;

    edit->lineEdit()->setText(path);
}
