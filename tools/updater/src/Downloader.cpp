/**
 * @file tools/updater/src/Downloader.cpp
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

#include "Downloader.h"

#include <PushIgnore.h>
#include <QDir>
#include <QUrl>
#include <QFile>
#include <QRegExp>
#include <QFileInfo>
#include <QMetaType>
#include <QCoreApplication>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <PopIgnore.h>

#if defined(COMP_HACK_HEADLESS) || defined(Q_OS_UNIX)
#define COMP_HACK_LOGSTDOUT
#endif // defined(COMP_HACK_HEADLESS) || defined(Q_OS_UNIX)

#ifdef COMP_HACK_LOGSTDOUT
#include <iostream>
#endif // COMP_HACK_LOGSTDOUT

#include <zlib.h>

bool Downloader::checkFile(const FileData *info)
{
    QString path = info->path;
    QRegExp compMatch("(.+).compressed");
    if( compMatch.exactMatch(info->path) )
        path = compMatch.cap(1);

    if(path == "ImagineUpdate.dat")
        return true;

    if(QFileInfo(path).size() != info->uncompressed_size)
        return false;

    QFile file(path);
    file.open(QIODevice::ReadOnly);

    QByteArray d = file.readAll();
    file.close();

    QString checksum = QCryptographicHash::hash(d,
        QCryptographicHash::Md5).toHex();

    if(checksum != info->uncompressed_hash)
        return false;

    return true;
}

QMap<QString, FileData*> Downloader::parseFileList(const QByteArray& d)
{
    QMap<QString, FileData*> files;

    QStringList lines = QString::fromUtf8(d).split("\n");
    foreach(QString line, lines)
    {
        QRegExp fileMatcher("FILE : (.+),([0-9a-fA-F]{32}),([0-9]+),"
            "([0-9a-fA-F]{32}),([0-9]+)");

        if( !fileMatcher.exactMatch( line.trimmed() ) )
            continue;

        FileData *info = new FileData;
        info->path = fileMatcher.cap(1).mid(2).replace("\\", "/");
        info->compressed_hash = fileMatcher.cap(2).toLower();
        info->compressed_size = fileMatcher.cap(3).toInt();
        info->uncompressed_hash = fileMatcher.cap(4).toLower();
        info->uncompressed_size = fileMatcher.cap(5).toInt();

        files[info->path] = info;
    }

    return files;
}

Downloader::Downloader(const QString& url, QObject *p) : QObject(p),
    mTotalFiles(0), mCurrentReq(0), mHaveVersion(false), mCurrentFile(0),
    mActiveRetries(0)
{
    mURL = url;
    mBare = false;
    mKill = false;
    mStatusCode = 0;
    mSaveFiles = true;

    mFileHash = new QCryptographicHash(QCryptographicHash::Md5);

#ifdef COMP_HACK_HEADLESS
    mUseClassic = true;
#else // COMP_HACK_HEADLESS
    mUseClassic = false;
#endif // COMP_HACK_HEADLESS

    QStringList args = QCoreApplication::instance()->arguments();

    while( !args.isEmpty() )
    {
        QString arg = args.takeFirst();

        if(arg == "--no-save")
            mSaveFiles = false;
        else if(arg == "--classic")
            mUseClassic = true;
        else if(arg == "--modern")
            mUseClassic = false;
        else if(arg == "--white")
            mWhiteList.append( args.takeFirst() );
        else if(arg == "--url")
            mURL = args.takeFirst();
        else if(arg == "--bare")
            mBare = true;
    }
}

Downloader::~Downloader()
{
    delete mFileHash;
    delete mConnection;
    delete mCurrentFile;

    foreach(FileData *info, mFiles)
        delete info;
}

void Downloader::triggerKill()
{
    emit updateKilled();

    deleteLater();

    mKill = true;
}

void Downloader::startUpdate()
{
    mConnection = new QNetworkAccessManager;

    if(mUseClassic)
    {
        startDownload(tr("%1/hashlist.dat").arg(mURL));
    }
    else
    {
        startDownload(tr("%1/hashlist.ver").arg(mURL));
    }
}

void Downloader::requestError(QNetworkReply::NetworkError code)
{
    (void)code;

    // If there was a timeout, try again before reporting the error.
    if(mActiveRetries && QNetworkReply::TimeoutError == mCurrentReq->error())
    {
        startDownload(mActiveURL, mActivePath);
        return;
    }

    auto errorString = mCurrentReq->errorString();

    if(!errorString.isEmpty())
    {
#ifdef COMP_HACK_LOGSTDOUT
        std::cout << "Download failed: "
            << errorString.toLocal8Bit().data() << std::endl;
#endif // COMP_HACK_LOGSTDOUT

        expressFinish(tr("Download failed: %1").arg(errorString));
    }
    else
    {
        mStatusCode = mCurrentReq->attribute(
            QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QString phrase = mCurrentReq->attribute(
            QNetworkRequest::HttpReasonPhraseAttribute).toString();

#ifdef COMP_HACK_LOGSTDOUT
        std::cout << "Download failed: Server returned status code "
            << mStatusCode << " " << phrase.toLocal8Bit().data() << std::endl;
#endif // COMP_HACK_LOGSTDOUT

        expressFinish(tr("Download failed: Server returned status "
            "code %1 %2").arg(mStatusCode).arg(phrase));
    }
}

void Downloader::requestReadyRead()
{
    QByteArray data = mCurrentReq->readAll();

    mFileHash->addData(data);
    mData.append(data);

    emit downloadProgressChanged(mData.size());
}

int Downloader::uncompressChunk(const void *src, void *dest,
    int in_size, int chunk_size) const
{
    z_stream strm;

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    strm.avail_in = static_cast<uInt>(in_size);
    strm.next_in = (Bytef*)src;

    if(inflateInit(&strm) != Z_OK)
        return 0;

    strm.avail_out = static_cast<uInt>(chunk_size);
    strm.next_out = (Bytef*)dest;

    if(inflate(&strm, Z_FINISH) != Z_STREAM_END)
        return 0;

    int written = chunk_size - static_cast<int>(strm.avail_out);

    if(inflateEnd(&strm) != Z_OK)
        return 0;

    return written;
}

QByteArray Downloader::uncompress(const QByteArray& data, int size) const
{
    char *out = new char[size];
    if(!out)
        return QByteArray();

    int ret = uncompressChunk(data.constData(), out, data.size(), size);
    if(ret != size)
    {
        delete[] out;

        return QByteArray();
    }

    QByteArray final(out, size);
    delete[] out;

    return final;
}

QByteArray Downloader::uncompressHashlist(const QByteArray& data) const
{
    char *out = new char[10485760]; // 10MiB
    if(!out)
        return QByteArray();

    int ret = uncompressChunk(data.constData(), out, data.size(), 10485760);
    if(ret <= 0)
    {
        delete[] out;

        return QByteArray();
    }

    QByteArray final(out, (int)strlen(out) + 1);
    delete[] out;

    return final;
}

void Downloader::requestFinished()
{
    if(0 == mCurrentReq)
    {
        return;
    }

    mCurrentReq->deleteLater();

    if(QNetworkReply::NoError != mCurrentReq->error())
    {
        requestError(mCurrentReq->error());
        mCurrentReq = 0;
        return;
    }

    QByteArray data = mCurrentReq->readAll();

    mFileHash->addData(data);
    mData.append(data);

    QString checksum = mFileHash->result().toHex();
    mFileHash->reset();

    mCurrentReq = 0;

    if( mPath.isEmpty() )
    {
        if(!mUseClassic && !mHaveVersion)
        {
            mServerVersion = QString::fromLatin1(mData).trimmed();

            QFile ver("ImagineUpdate2.ver");
            ver.open(QIODevice::ReadOnly);

            mLastVersion = QString::fromLatin1( ver.readLine() ).trimmed();

            mHaveVersion = true;

            if(mLastVersion.size() > 20 && mServerVersion == mLastVersion)
            {
                emit updateFinished();

                return;
            }

            // Now download the hashlist
            startDownload(QString("%1/hashlist.dat.compressed").arg(mURL));

            return;
        }

        QFile last("ImagineUpdate2.dat");
        last.open(QIODevice::ReadOnly);

        if(!mUseClassic)
        {
            mData = uncompressHashlist(mData);
        }

        mOldFiles = parseFileList( last.readAll() );
        mFiles = parseFileList(mData).values();

        mTotalFiles = mFiles.count();

        emit totalFilesChanged(mTotalFiles);

#ifdef COMP_HACK_HEADLESS
        if(mSaveFiles)
        {
            QFile out("hashlist.dat");
            out.open(QIODevice::WriteOnly);
            out.write(mData);
            out.close();
        }
#endif // COMP_HACK_HEADLESS

        last.close();
        last.remove();
    }
    else
    {
        emit downloadProgressChanged(mData.size());

        if(!mCurrentFile)
        {
#ifdef COMP_HACK_LOGSTDOUT
            std::cout << "Download failed: No current file" << std::endl;
#endif // COMP_HACK_LOGSTDOUT

            return expressFinish("Download failed: No current file");
        }

        if(mData.size() != mCurrentFile->compressed_size)
        {
#ifdef COMP_HACK_LOGSTDOUT
            std::cout << "Download failed: Compressed size "
                << "does not match" << std::endl;
#endif // COMP_HACK_LOGSTDOUT

            return expressFinish("Download failed: Compressed "
                "size does not match");
        }

        if(checksum != mCurrentFile->compressed_hash)
        {
#ifdef COMP_HACK_LOGSTDOUT
            std::cout << "Download failed: Invalid compressed "
                << "hash detected" << std::endl;
#endif // COMP_HACK_LOGSTDOUT

            return expressFinish("Download failed: Invalid "
                "compressed hash detected");
        }

        QRegExp compMatch("(.+).compressed");
        if( compMatch.exactMatch(mPath) )
        {
#ifdef COMP_HACK_HEADLESS
            if(mSaveFiles)
            {
                QFile file(mPath);
                file.open(QIODevice::WriteOnly);
                file.write(mData);
                file.close();
            }
#endif // COMP_HACK_HEADLESS

            QByteArray uncomp = uncompress(mData,
                mCurrentFile->uncompressed_size);

            if(uncomp.size() != mCurrentFile->uncompressed_size)
            {
#ifdef COMP_HACK_LOGSTDOUT
                std::cout << "Download failed: Uncompressed size "
                    << "does not match" << std::endl;
#endif // COMP_HACK_LOGSTDOUT

                return expressFinish("Download failed: Uncompressed "
                    "size does not match");
            }

            checksum = QCryptographicHash::hash(uncomp,
                QCryptographicHash::Md5).toHex();

            if(checksum != mCurrentFile->uncompressed_hash)
            {
#ifdef COMP_HACK_LOGSTDOUT
                std::cout << "Download failed: Invalid uncompressed "
                    << "hash detected" << std::endl;
#endif // COMP_HACK_LOGSTDOUT

                return expressFinish("Download failed: Invalid "
                    "uncompressed hash detected");
            }

            if(!mBare)
            {
                QFile file( compMatch.cap(1) );
                file.open(QIODevice::WriteOnly);
                file.write(uncomp);
                file.close();
            }
        }
        else
        {
            QFile file(mPath);
            file.open(QIODevice::WriteOnly);
            file.write(mData);
            file.close();
        }

        if(mSaveFiles)
        {
            QFile last("ImagineUpdate2.dat");
            last.open(QIODevice::WriteOnly | QIODevice::Append);
            last.write( QString("FILE : ./%1,%2,%3,%4,%5\n").arg(
                mCurrentFile->path).arg(mCurrentFile->compressed_hash).arg(
                mCurrentFile->compressed_size).arg(
                mCurrentFile->uncompressed_hash).arg(
                mCurrentFile->uncompressed_size).toUtf8() );
            last.close();
        }
    }

    advanceToNextFile();
}

void Downloader::expressFinish(const QString& msg)
{
    QFile last("ImagineUpdate2.dat");
    if(mSaveFiles)
        last.open(QIODevice::WriteOnly | QIODevice::Append);

    while( !mFiles.isEmpty() )
    {
        FileData *info = mFiles.takeFirst();

        if( !mWhiteList.isEmpty() )
        {
            bool good = false;
            foreach(QString pattern, mWhiteList)
            {
                if(info->path.left( pattern.length() ) != pattern)
                    continue;

                good = true;
                break;
            }

            if(!good)
                continue;
        }

        if( !mOldFiles.contains(info->path) )
            continue;

        FileData *old = mOldFiles.value(info->path);
        if(old->uncompressed_hash != info->uncompressed_hash)
            continue;

        if(mSaveFiles)
        {
            last.write( QString("FILE : ./%1,%2,%3,%4,%5\n").arg(
                info->path).arg(info->compressed_hash).arg(
                info->compressed_size).arg(
                info->uncompressed_hash).arg(
                info->uncompressed_size).toUtf8() );
        }
    }

    // Make sure this is gone
    QFile("ImagineUpdate2.ver").remove();

    // Emit the error message
    if( !msg.isEmpty() )
        emit errorMessage(msg);

    // Exit the thread so the application can exit
    emit updateFinished();
}

void Downloader::advanceToNextFile()
{
    QFile last("ImagineUpdate2.dat");
    if(mSaveFiles)
        last.open(QIODevice::WriteOnly | QIODevice::Append);

    bool startedFile = false;

    while( !mFiles.isEmpty() )
    {
        if(mKill)
            return expressFinish();

        FileData *info = mFiles.takeFirst();

        if( !mWhiteList.isEmpty() )
        {
            bool good = false;
            foreach(QString pattern, mWhiteList)
            {
                if(info->path.left( pattern.length() ) != pattern)
                    continue;

                good = true;
                break;
            }

            if(!good)
                continue;
        }

        if( mOldFiles.contains(info->path) )
        {
            FileData *old = mOldFiles.value(info->path);
            if(old->uncompressed_hash == info->uncompressed_hash)
            {
                if(mSaveFiles)
                {
                    last.write( QString("FILE : ./%1,%2,%3,%4,%5\n").arg(
                        info->path).arg(info->compressed_hash).arg(
                        info->compressed_size).arg(
                        info->uncompressed_hash).arg(
                        info->uncompressed_size).toUtf8() );
                }

                continue;
            }
        }
        else
        {
            if( checkFile(info) )
            {
                if(mSaveFiles)
                {
                    last.write( QString("FILE : ./%1,%2,%3,%4,%5\n").arg(
                        info->path).arg(info->compressed_hash).arg(
                        info->compressed_size).arg(
                        info->uncompressed_hash).arg(
                        info->uncompressed_size).toUtf8() );
                }

                QString filename = info->path;
                filename = filename.replace("/", "\\");
                if(filename.right(11) == ".compressed")
                    filename = filename.left(filename.length() - 11);

                emit currentFileChanged(mTotalFiles - mFiles.count());
                emit statusChanged(filename);
                emit downloadSizeChanged(info->compressed_size);
                emit downloadProgressChanged(info->compressed_size);

                continue;
            }
        }

        QDir::current().mkpath( QFileInfo(info->path).path() );

        delete mCurrentFile;
        mCurrentFile = info;

        QString filename = info->path;
        filename = filename.replace("/", "\\");
        if(filename.right(11) == ".compressed")
            filename = filename.left(filename.length() - 11);

        emit currentFileChanged(mTotalFiles - mFiles.count());
        emit statusChanged(filename);
        emit downloadSizeChanged(info->compressed_size);
        emit downloadProgressChanged(0);

        // Download the new version
        startDownload(QString("%1/%2").arg(mURL).arg(
            info->path), info->path);

        startedFile = true;

        break;
    }

    if( !startedFile && mFiles.isEmpty() )
    {
#ifdef COMP_HACK_HEADLESS
        std::cout << "Done!" << std::endl;
#endif // COMP_HACK_HEADLESS

        if(mSaveFiles && !mUseClassic)
        {
            QFile ver("ImagineUpdate2.ver");
            ver.open(QIODevice::WriteOnly);
            ver.write(mServerVersion.toLatin1());
            ver.close();
        }

        emit updateFinished();
    }
}

void Downloader::startDownload(const QString& url, const QString& path)
{
    mStatusCode = 0;

    if(mActiveURL == url || mActivePath == path)
    {
        mActiveRetries--;
    }
    else
    {
        mActiveRetries = 5;
        mActiveURL = url;
        mActivePath = path;
    }

#ifdef COMP_HACK_HEADLESS
    if( path.isEmpty() )
        std::cout << "Downloading: hashlist.dat" << std::endl;
    else
        std::cout << "Downloading: " << path.toLocal8Bit().data() << std::endl;
#endif // COMP_HACK_HEADLESS

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "ImagineUpdate");

    mData.clear();

    mPath = path;
    mFileHash->reset();

    mCurrentReq = mConnection->get(request);

    connect(mCurrentReq, SIGNAL(error(QNetworkReply::NetworkError)), this,
        SLOT(requestError(QNetworkReply::NetworkError)));
    connect(mCurrentReq, SIGNAL(readyRead()), this, SLOT(requestReadyRead()));
    connect(mCurrentReq, SIGNAL(finished()), this, SLOT(requestFinished()));
}

void Downloader::setURL(const QString& url)
{
    mURL = url;
}
