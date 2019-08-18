/**
 * @file tools/updater/src/LanguageSelection.cpp
 * @ingroup tools
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief GUI for the language selection.
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

#include "LanguageSelection.h"
#include "Updater.h"

#include <PushIgnore.h>
#include <QDir>
#include <QFile>
#include <QLibraryInfo>
#include <QMessageBox>
#include <QTranslator>
#include <PopIgnore.h>

extern QList<QTranslator*> gTranslators;

LanguageSelection::LanguageSelection(QWidget *p) : QDialog(p,
    Qt::WindowSystemMenuHint | Qt::WindowTitleHint |
    Qt::WindowCloseButtonHint)
{
    ui.setupUi(this);

    // We are modal and should delete when closed.
    setWindowModality(Qt::WindowModal);
    setAttribute(Qt::WA_DeleteOnClose);

    QStringList langs = QDir(QLibraryInfo::location(
        QLibraryInfo::TranslationsPath)).entryList(
        QStringList() << "updater_*.qm");

    foreach(QString lang, langs)
    {
        lang = lang.replace("updater_", QString()).replace(
            ".qm", QString());

        ui.langCombo->addItem(QLocale(lang).nativeLanguageName(), lang);
    }

    connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(Save()));
    connect(ui.langCombo, SIGNAL(currentIndexChanged(int)),
        this, SLOT(LanguageChanged()));

    ui.langCombo->setCurrentText(
        QLocale::system().nativeLanguageName());
}

LanguageSelection::~LanguageSelection()
{
}

void LanguageSelection::LanguageChanged()
{
    QTranslator *pTranslator = new QTranslator;

    QString locale = ui.langCombo->currentData().toString();

    pTranslator->load("qt_" + locale.split("_").first(),
        QLibraryInfo::location(QLibraryInfo::TranslationsPath));

    if(pTranslator->load("updater_" + locale,
        QLibraryInfo::location(QLibraryInfo::TranslationsPath)))
    {
        foreach(QTranslator *pTrans, gTranslators)
        {
            QCoreApplication::instance()->removeTranslator(pTrans);
        }

        gTranslators.push_back(pTranslator);

        QCoreApplication::instance()->installTranslator(pTranslator);
    }
    else
    {
        delete pTranslator;
    }
}

void LanguageSelection::Save()
{
    QString locale = ui.langCombo->currentData().toString();

    QFile file("ImagineUpdate.lang");

    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::critical(this, tr("Save Error"),
            tr("Failed to save the language selection!"));

        return;
    }

    file.write(locale.toLocal8Bit());
    file.close();

    if(!QFileInfo(tr("translations/ImagineUpdate_%1.dat").arg(
        locale)).isReadable())
    {
        QMessageBox::critical(this, tr("Save Error"),
            tr("ImagineUpdate_%1.dat does not exist in the "
                "translations directory!").arg(locale));

        return;
    }

    if(QFileInfo("ImagineUpdate.dat").exists() &&
        !QFile::remove("ImagineUpdate.dat"))
    {
        QMessageBox::critical(this, tr("Save Error"),
            tr("Failed to delete existing ImagineUpdate.dat!"));

        return;
    }

    if(!QFile::copy(tr("translations/ImagineUpdate_%1.dat").arg(
        locale), "ImagineUpdate.dat"))
    {
        QMessageBox::critical(this, tr("Save Error"),
            tr("Failed to save the updater URL!"));

        return;
    }

    (new Updater)->show();

    close();
}

void LanguageSelection::changeEvent(QEvent *pEvent)
{
    if(QEvent::LanguageChange == pEvent->type())
    {
        ui.retranslateUi(this);
    }
}
