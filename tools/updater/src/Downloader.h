/**
 * @file tools/updater/src/Downloader.h
 * @ingroup tools
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Main download thread.
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

#ifndef TOOLS_UPDATER_SRC_DOWNLOADER_H
#define TOOLS_UPDATER_SRC_DOWNLOADER_H

#include <PushIgnore.h>
#include <QMap>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QCryptographicHash>
#include <QNetworkReply>
#include <PopIgnore.h>

class QNetworkAccessManager;

class FileData
{
public:
    QString path;
    QString compressed_hash;
    QString uncompressed_hash;

    int compressed_size;
    int uncompressed_size;
};

class Downloader : public QObject
{
    Q_OBJECT

public:
    Downloader(const QString& url, QObject *parent = 0);
    ~Downloader();

    void triggerKill();
    void setURL(const QString& url);

signals:
    void updateKilled();
    void updateFinished();
    void statusChanged(const QString& msg);

    void totalFilesChanged(int total);
    void currentFileChanged(int current);

    void downloadSizeChanged(int total);
    void downloadProgressChanged(int wrote);

    void errorMessage(const QString& msg);

public slots:
    void startUpdate();

protected slots:
    void expressFinish(const QString& msg = QString());
    void advanceToNextFile();

    void requestError(QNetworkReply::NetworkError code);
    void requestReadyRead();
    void requestFinished();

protected:
    void startDownload(const QString& url, const QString& path = QString());

    QByteArray uncompress(const QByteArray& data, int size) const;
    QByteArray uncompressHashlist(const QByteArray& data) const;
    int uncompressChunk(const void *src, void *dest,
        int in_size, int chunk_size) const;
    QMap<QString, FileData*> parseFileList(const QByteArray& d);
    bool checkFile(const FileData *info);

    int mStatusCode;
    int mTotalFiles;

    QNetworkReply *mCurrentReq;

    QCryptographicHash *mFileHash;

    QString mURL;
    QString mPath;
    QByteArray mData;

    QString mServerVersion;
    QString mLastVersion;
    bool mHaveVersion;

    bool mBare;
    bool mKill;
    bool mSaveFiles;
    bool mUseClassic;
    QStringList mWhiteList;

    QNetworkAccessManager *mConnection;
    FileData *mCurrentFile;

    QList<FileData*> mFiles;
    QMap<QString, FileData*> mOldFiles;

    QString mActiveURL;
    QString mActivePath;
    int mActiveRetries;
};

#endif // TOOLS_UPDATER_SRC_DOWNLOADER_H
