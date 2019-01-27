/**
 * @file tools/updater/src/Updater.cpp
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

#include "Updater.h"
#include "Downloader.h"
#include "Options.h"

#include <PushIgnore.h>
#include <QMenu>
#include <QMessageBox>
#include <QApplication>
#include <QStringList>
#include <QSettings>
#include <QProcess>
#include <QFile>
#include <QDate>
#include <QDir>
#include <PopIgnore.h>

#include <iostream>

Updater::Updater(QWidget *p) : QWidget(p), mDone(false)
{
    QString settingsPath = "ImagineUpdate.dat";

    if(QFileInfo("ImagineUpdate-user.dat").exists())
    {
        settingsPath = "ImagineUpdate-user.dat";
    }

    QSettings settings(settingsPath, QSettings::IniFormat);

    mURL = settings.value("Setting/BaseURL1").toString();
    mWebsite = settings.value("Setting/Information").toString();

    ui.setupUi(this);
    ui.playButton->setEnabled(false);
    ui.website->load(mWebsite);

    ui.fileProgress->setMaximum(100);
    ui.fileProgress->setValue(100);

    ui.totalProgress->setMaximum(1);
    ui.totalProgress->setValue(30000);

    setFixedSize(sizeHint());

    mDL = new Downloader(mURL);
    mDL->moveToThread(&mDownloadThread);

    connect(&mDownloadThread, SIGNAL(started()), mDL, SLOT(startUpdate()));
    connect(mDL, SIGNAL(updateFinished()), &mDownloadThread, SLOT(quit()));
    connect(mDL, SIGNAL(updateKilled()), &mDownloadThread, SLOT(quit()));
    connect(mDL, SIGNAL(updateFinished()), this, SLOT(unlock()));
    connect(mDL, SIGNAL(statusChanged(const QString&)),
        ui.statusLabel, SLOT(setText(const QString&)));
    connect(mDL, SIGNAL(totalFilesChanged(int)),
        ui.totalProgress, SLOT(setMaximum(int)));
    connect(mDL, SIGNAL(currentFileChanged(int)),
        ui.totalProgress, SLOT(setValue(int)));
    connect(mDL, SIGNAL(downloadSizeChanged(int)),
        ui.fileProgress, SLOT(setMaximum(int)));
    connect(mDL, SIGNAL(downloadProgressChanged(int)),
        ui.fileProgress, SLOT(setValue(int)));
    connect(mDL, SIGNAL(errorMessage(const QString&)),
        this, SLOT(errorMessage(const QString&)));

    connect(ui.settingsButton, SIGNAL(clicked(bool)),
        this, SLOT(showSettings()));
    connect(ui.screenshotsButton, SIGNAL(clicked(bool)),
        this, SLOT(showScreenshots()));
    connect(ui.diagButton, SIGNAL(clicked(bool)),
        this, SLOT(showDXDiag()));
    connect(ui.checkButton, SIGNAL(clicked(bool)),
        this, SLOT(recheck()));
    connect(ui.retryButton, SIGNAL(clicked(bool)),
        this, SLOT(retry()));

    mDownloadThread.start();
}

Updater::~Updater()
{
    foreach(VersionData *d, mVersionMap)
        delete d;
}

void Updater::unlock()
{
    QDate aprilFools = QDate::currentDate();

    if(aprilFools.month() == 4 && aprilFools.day() == 1)
    {
        QMessageBox::critical(this, "Error", "mismatch occured in data file");

        QMessageBox::information(this, "April Fools",
            "OK, no error actually occured, but do you remember when you "
            "used to get these damn errors? Pathetic they never figured it "
            "out, isn't it. Enjoy your april fools day! -COMP_hack Team");
    }

    mDone = true;

    ui.fileProgress->setMaximum(100);
    ui.fileProgress->setValue(100);

    ui.totalProgress->setMaximum(100);
    ui.totalProgress->setValue(100);

    ui.statusLabel->setText("Update Complete");

    QMenu *playMenu = new QMenu;

    foreach(VersionData *d, mVersionMap)
        delete d;

    mVersionMap.clear();

    QString versionPath = "VersionData.txt";

    if(QFileInfo("VersionData-user.txt").exists())
    {
        versionPath = "VersionData-user.txt";
    }

    QFile versionMap(versionPath);

    if( versionMap.open(QIODevice::ReadOnly | QIODevice::Text) )
    {
        QString versionLine;

        while( !versionMap.atEnd() )
        {
            versionLine = QString::fromUtf8(versionMap.readLine()).trimmed();

            if(versionLine.count() < 1 || versionLine.left(1) == "#")
                continue;
            if(versionLine.toLower() == "[versions]")
                break;

            QMessageBox::critical(this, tr("VersionData.txt Error"),
                tr("The first line of the file was not: [versions]"));

            return qApp->quit();
        }

        VersionData version;

        while( !versionMap.atEnd() )
        {
            versionLine = QString::fromUtf8(versionMap.readLine()).trimmed();

            if(versionLine.count() < 1 || versionLine.left(1) == "#")
                continue;
            if(versionLine.toLower().left(1) == "[")
                break;

            QStringList line = versionLine.split('=');

            if(line.count() != 2)
            {
                QMessageBox::critical(this, tr("VersionData.txt Error"),
                    tr("Invalid line found in versions section: %1").arg(versionLine));

                return qApp->quit();
            }

            QString key = line.at(0).trimmed().toLower();
            QString value = line.at(1).trimmed();

            if(key == "title")
            {
                if( !version.title.isEmpty() )
                {
                    QMessageBox::critical(this, tr("VersionData.txt Error"),
                        tr("Duplicate title value found"));

                    return qApp->quit();
                }

                version.title = value;
            }
            else if(key == "server")
            {
                if( !version.server.isEmpty() )
                {
                    QMessageBox::critical(this, tr("VersionData.txt Error"),
                        tr("Duplicate server value found"));

                    return qApp->quit();
                }

                version.server = value;
            }
            else if(key == "tag")
            {
                value = value.toLower();

                if( !version.tag.isEmpty() )
                {
                    QMessageBox::critical(this, tr("VersionData.txt Error"),
                        tr("Duplicate tag value found"));

                    return qApp->quit();
                }

                if( mVersionMap.contains(value) )
                {
                    QMessageBox::critical(this, tr("VersionData.txt Error"),
                        tr("Non-unique tag value found"));

                    return qApp->quit();
                }

                version.tag = value;
            }
            else
            {
                QMessageBox::critical(this, tr("VersionData.txt Error"),
                    tr("Version contains invalid value"));

                return qApp->quit();
            }

            if( !version.title.isEmpty() && !version.server.isEmpty() &&
                !version.tag.isEmpty() )
            {
                VersionData *d = new VersionData;
                d->title = version.title;
                d->server = version.server;
                d->tag = version.tag;

                version.title.clear();
                version.server.clear();
                version.tag.clear();

                mVersionMap[d->tag] = d;

                QAction *action = playMenu->addAction(d->title);
                action->setData(d->tag);

                connect(action, SIGNAL(triggered(bool)),
                    this, SLOT(startGame()));
            }
        }

        if( !version.title.isEmpty() || !version.server.isEmpty() ||
            !version.tag.isEmpty() )
        {
            QMessageBox::critical(this, tr("VersionData.txt Error"),
                tr("Version missing one or more of: title, server, tag"));

            return qApp->quit();
        }

        VersionData *d = 0;

        QRegExp tagMatcher("\\[([a-zA-Z0-9]+)\\]");

        while(true)
        {
            if(versionLine.count() < 1 || versionLine.left(1) == "#")
            {
                if( versionMap.atEnd() )
                    break;

                versionLine = QString::fromUtf8(
                    versionMap.readLine()).trimmed();

                continue;
            }

            if( tagMatcher.exactMatch(versionLine) || !d )
            {
                QString tag = tagMatcher.cap(1).toLower();

                if( !mVersionMap.contains(tag) )
                {
                    QMessageBox::critical(this, tr("VersionData.txt Error"),
                        tr("Section contains invalid tag name"));

                    return qApp->quit();
                }

                d = mVersionMap.value(tag);

                if( versionMap.atEnd() )
                    break;

                versionLine = QString::fromUtf8(
                    versionMap.readLine()).trimmed();

                continue;
            }

            QStringList line = versionLine.split('=');

            if(line.count() > 2)
            {
                QMessageBox::critical(this, tr("VersionData.txt Error"),
                    tr("Invalid line found in file list section"));

                return qApp->quit();
            }

            QString file = line.at(0).trimmed();
            QString tag = d->tag;

            if(line.count() > 1)
                tag = line.at(1).trimmed();

            if( d->files.contains(file) )
            {
                QMessageBox::critical(this, tr("VersionData.txt Error"),
                    tr("Duplicate file '%1' found for tag '%2'").arg(
                    file).arg(d->tag));

                return qApp->quit();
            }

            d->files[file] = tag;

            if( versionMap.atEnd() )
                break;

            versionLine = QString::fromUtf8(versionMap.readLine()).trimmed();
        }
        while( !versionMap.atEnd() );
    }

    ui.playButton->setMenu(playMenu);
    ui.playButton->setEnabled(true);
    ui.settingsButton->setEnabled(true);
}

bool Updater::copyFile(const QString& src, const QString& dest)
{
    QFile in(src), out(dest);

    if( !in.open(QIODevice::ReadOnly) || !out.open(QIODevice::WriteOnly) )
        return false;

    if( out.write( in.readAll() ) != QFileInfo(src).size() )
        return false;

    return true;
}

void Updater::startGame()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if(action)
    {
        QFile serverInfo("ImagineClient.dat");
        serverInfo.open(QIODevice::WriteOnly);

        QString tag = action->data().toString();
        if( !mVersionMap.contains(tag) )
            return;

        VersionData *ver = mVersionMap.value(tag);
        if(!ver)
            return;

        QStringList serv = ver->server.split(':');

        serverInfo.write( QString("-ip %1\r\n").arg(serv.at(0)).toUtf8() );
        serverInfo.write( QString("-port %1\r\n").arg(serv.at(1)).toUtf8() );
        serverInfo.close();

        QMapIterator<QString, QString> it(ver->files);

        while( it.hasNext() )
        {
            it.next();

            QString file = it.key();
            QString suffix = it.value();

            QString source = tr("%1/%2.%3").arg(
                qApp->applicationDirPath()).arg(file).arg(suffix);
            QString dest = tr("%1/%2").arg(
                qApp->applicationDirPath()).arg(file);

            if( !copyFile(source, dest) )
            {
                QMessageBox::critical(this, tr("Updater Error"),
                    tr("Failed to patch %1").arg(file));

                return qApp->quit();
            }
        }
    }

#ifdef Q_OS_WIN32
    QProcess::startDetached("ImagineClient.exe");
#else
    QProcess::startDetached("env WINEPREFIX=\"/home/erikku/.wine\" wine ImagineClient.exe");
#endif // Q_OS_WIN32

    qApp->quit();
}

void Updater::closeEvent(QCloseEvent *evt)
{
    Q_UNUSED(evt)

    if(!mDone)
    {
        mDL->triggerKill();

        mDownloadThread.quit();
        mDownloadThread.wait();
    }

    qApp->quit();
}

void Updater::showSettings()
{
#ifdef Q_OS_WIN32
    (new Options(this))->show();
#else
    QProcess::startDetached("env WINEPREFIX=\"/home/erikku/.wine\" wine "
        "\"C:\\AeriaGames\\MegaTen\\ImagineOption.exe\"");
#endif // Q_OS_WIN32
}

void Updater::showScreenshots()
{
    QString path = QString("%1/screenshot").arg( qApp->applicationDirPath() );
    path = QDir::toNativeSeparators(path);

    QProcess::startDetached("explorer", QStringList() << path);
}

void Updater::showDXDiag()
{
    QProcess::startDetached("dxdiag", QStringList() << "/whql:off");
}

void Updater::recheck()
{
    QString path = QString("%1/ImagineUpdate2.dat").arg(
        qApp->applicationDirPath() );

    QFile(path).remove();

    path = QString("%1/ImagineUpdate2.ver").arg(
        qApp->applicationDirPath() );

    QFile(path).remove();

    retry();
}

void Updater::retry()
{
    mDone = false;

    bool block = mDL->blockSignals(true);

    mDL->triggerKill();

    mDownloadThread.quit();
    mDownloadThread.wait();

    mDL->blockSignals(block);

    ui.settingsButton->setEnabled(false);
    ui.playButton->setEnabled(false);

    ui.fileProgress->setMaximum(100);
    ui.fileProgress->setValue(100);

    ui.totalProgress->setMaximum(1);
    ui.totalProgress->setValue(30000);

    ui.statusLabel->setText("Please wait...");

    mDL = new Downloader(mURL);
    mDL->moveToThread(&mDownloadThread);

    connect(&mDownloadThread, SIGNAL(started()), mDL, SLOT(startUpdate()));
    connect(mDL, SIGNAL(updateFinished()), &mDownloadThread, SLOT(quit()));
    connect(mDL, SIGNAL(updateKilled()), &mDownloadThread, SLOT(quit()));
    connect(mDL, SIGNAL(updateFinished()), this, SLOT(unlock()));
    connect(mDL, SIGNAL(statusChanged(const QString&)),
        ui.statusLabel, SLOT(setText(const QString&)));
    connect(mDL, SIGNAL(totalFilesChanged(int)),
        ui.totalProgress, SLOT(setMaximum(int)));
    connect(mDL, SIGNAL(currentFileChanged(int)),
        ui.totalProgress, SLOT(setValue(int)));
    connect(mDL, SIGNAL(downloadSizeChanged(int)),
        ui.fileProgress, SLOT(setMaximum(int)));
    connect(mDL, SIGNAL(downloadProgressChanged(int)),
        ui.fileProgress, SLOT(setValue(int)));
    connect(mDL, SIGNAL(errorMessage(const QString&)),
        this, SLOT(errorMessage(const QString&)));

    mDownloadThread.start();
}

void Updater::errorMessage(const QString& msg)
{
    QMessageBox::critical(this, tr("Updater Error"), msg);

    qApp->quit();
}

void Updater::ReloadURL()
{
    QString settingsPath = "ImagineUpdate.dat";

    if(QFileInfo("ImagineUpdate-user.dat").exists())
    {
        settingsPath = "ImagineUpdate-user.dat";
    }

    QSettings settings(settingsPath, QSettings::IniFormat);

    mURL = settings.value("Setting/BaseURL1").toString();
    mWebsite = settings.value("Setting/Information").toString();

    ui.website->load(mWebsite);

    ui.settingsButton->setEnabled(false);
    ui.playButton->setEnabled(false);

    mDL->setURL(mURL);
    mDownloadThread.start();

    ui.retranslateUi(this);
}
