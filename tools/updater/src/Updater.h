/**
 * @file tools/updater/src/Updater.h
 * @ingroup tools
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief GUI for the updater.
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

#ifndef TOOLS_UPDATER_SRC_UPDATER_H
#define TOOLS_UPDATER_SRC_UPDATER_H

#include <PushIgnore.h>
#include "ui_Updater.h"

#include <QMap>
#include <QThread>
#include <PopIgnore.h>

class Downloader;

class VersionData
{
public:
    QString title;
    QString server;
    QString tag;

    QMap<QString, QString> files;
};

class Updater : public QWidget
{
    Q_OBJECT

public:
    Updater(QWidget *parent = 0);
    ~Updater();

    void ReloadURL();

protected slots:
    void unlock();
    void startGame();
    void showSettings();
    void showScreenshots();
    void showDXDiag();
    void recheck();
    void retry();

    void errorMessage(const QString& msg);

protected:
    virtual void closeEvent(QCloseEvent *event);

    bool copyFile(const QString& src, const QString& dest);

    bool mDone;

    QString mURL;
    QString mWebsite;

    Downloader *mDL;
    QThread mDownloadThread;

    QMap<QString, VersionData*> mVersionMap;

    Ui::Updater ui;
};

#endif // TOOLS_UPDATER_SRC_UPDATER_H
