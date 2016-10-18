/**
 * @file tools/capgrep/src/PacketListModel.cpp
 * @ingroup capgrep
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Model to show a list of packets.
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

#include "PacketListModel.h"
#include "PacketData.h"

#include <PushIgnore.h>
#include <QFile>

#include <QBrush>
#include <QColor>

#include <QDomDocument>
#include <QDomElement>
#include <PopIgnore.h>

QHash<uint16_t, PacketInfo*> PacketListModel::mPacketInfo;

PacketListModel::PacketListModel(QObject *p) : QAbstractListModel(p),
    mPacketLimit(0)
{
    if(mPacketInfo.isEmpty())
        loadPacketInfo();

    mIcons << QIcon(":/a.png") << QIcon(":/b.png") << QIcon(":/c.png")
        << QIcon(":/d.png") << QIcon(":/e.png") << QIcon(":/f.png");
}

int PacketListModel::rowCount(const QModelIndex& p) const
{
    Q_UNUSED(p);

    return mPacketData.count();
}

QVariant PacketListModel::data(const QModelIndex& idx, int role) const
{
    if( !idx.isValid() )
        return QVariant();

    PacketData *d = packetAt(idx.row());
    if(!d)
        return QVariant();

    switch(role)
    {
        case Qt::DisplayRole:
        {
            const PacketInfo *info = getPacketInfo(d->cmd);

            return info ? info->name : d->text;
        }
        case Qt::ToolTipRole:
        {
            const PacketInfo *info = getPacketInfo(d->cmd);

            return info ? info->desc : d->desc;
        }
        case Qt::ForegroundRole:
        {
            if(d->source == 0)
                return QBrush( QColor(d->seq % 2 ? 128 : 255, 0, 0) );
            else
                return QBrush( QColor(0, 0, d->seq % 2 ? 128 : 255) );
        }
        case Qt::DecorationRole:
        {
            if(d->client >= 0 && d->client < 6)
                return mIcons.at(d->client);

            break;
        }
        default:
            break;
    }

    return QVariant();
}

PacketData* PacketListModel::packetBefore(int idx) const
{
    // Sanity check the bounds of the index.
    if( idx < 0 || idx >= mPacketData.count() )
        return 0;

    // Get the packet we want a previous version of.
    PacketData *first = mPacketData.at(idx);

    // Sanity check that we found it.
    if(!first)
        return 0;

    // Look for a previous packet.
    for(int i = idx; i > 0; i--)
    {
        // Get the packet to check.
        PacketData *prev = mPacketData.at(i - 1);

        // If the packet has the same command code, we found it.
        if(prev && prev->cmd == first->cmd)
            return prev;
    }

    // No previous packet was found.
    return 0;
}

PacketData* PacketListModel::packetAt(int idx) const
{
    if( idx < 0 || idx >= mPacketData.count() )
        return 0;

    return mPacketData.at(idx);
}

void PacketListModel::setPacketData(const QList<PacketData*>& packetData)
{
    beginResetModel();

    foreach(PacketData *d, mPacketData)
        delete d;

    mPacketData.clear();
    mPacketData = packetData;

    endResetModel();
}

void PacketListModel::addPacketData(const QList<PacketData*>& packetData)
{
    int32_t inSize = packetData.count();
    int32_t inStart = 0;

    if(mPacketLimit && inSize > mPacketLimit)
    {
        inStart = inSize - mPacketLimit;
        inSize = mPacketLimit;
    }

    int32_t newSize = mPacketData.count() + inSize;

    if(mPacketLimit && newSize > mPacketLimit)
    {
        int32_t removeCount = newSize - mPacketLimit;

        beginRemoveRows(QModelIndex(), 0, removeCount - 1);

        while(removeCount-- > 0)
            delete mPacketData.takeFirst();

        endRemoveRows();
    }

    beginInsertRows(QModelIndex(), mPacketData.count(),
        mPacketData.count() + inSize - 1);

    mPacketData.append(packetData.mid(inStart));

    endInsertRows();
}

void PacketListModel::clear()
{
    foreach(PacketData *d, mPacketData)
        delete d;

    mPacketData.clear();

    beginResetModel();
    endResetModel();
}

QModelIndex PacketListModel::modelIndex(int idx) const
{
    if( idx < 0 || idx >= mPacketData.count() )
        return QModelIndex();

    return createIndex(idx, 0);
}

void PacketListModel::reset()
{
    beginResetModel();
    endResetModel();
}

void PacketListModel::loadPacketInfo()
{
    // Remove any old entries
    foreach(PacketInfo *info, mPacketInfo)
        delete info;

    // Clear the hash of all entries
    mPacketInfo.clear();

    // Open the file
    QFile infoFile(":/packets.xml");
    infoFile.open(QIODevice::ReadOnly);

    // Parse the file
    QDomDocument infoDOM("packets");
    if(!infoDOM.setContent(&infoFile))
        return;

    // Close the file
    infoFile.close();

    // Find all the packet entries
    QDomNodeList infoList = infoDOM.elementsByTagName("packet");

    for(int i = 0; i < infoList.count(); i++)
    {
        // Get the element for this packet
        QDomElement infoNode = infoList.at(i).toElement();

        // If the element isn't valid, move on to the next packet
        if(infoNode.isNull())
            continue;

        // If the packet code converted to a number right
        bool ok = false;

        // Code of the packet
        uint16_t code = 0;

        // Get the code as a string
        QString codeStr = infoNode.attribute("code").trimmed();

        // toUShort doesn't actually work with the 0x prefix so check for it
        // and convert the code to a integer
        if(codeStr.left(2).toLower() == "0x")
            code = codeStr.mid(2).toUShort(&ok, 16);
        else
            code = codeStr.toUShort(&ok, 10);

        // Move on if this packet failed
        if(!ok)
            continue;

        // Only add packet info once
        if(mPacketInfo.contains(code))
            continue;

        // Origin of the packet (0=client, 1=server)
        uint8_t origin = 0;

        // Get the origin of the packet
        QString originStr = infoNode.attribute("origin").trimmed().toLower();
        if(originStr == "server")
            origin = 1;
        else if(originStr == "client")
            origin = 0;
        else
            continue;

        // Make sure the name for this packet isn't empty
        QString name = infoNode.attribute("name").trimmed();
        if(name.isEmpty())
            continue;

        // Create the packet info object
        PacketInfo *info = new PacketInfo;
        info->code = code;
        info->origin = origin;
        info->name = QString("%1 (0x%2)").arg(name).arg(QString("%1").arg(
            code, 4, 16, QLatin1Char('0')).toUpper());
        info->desc = infoNode.text().trimmed();

        // Add the packet info to the hash
        mPacketInfo[code] = info;
    }
}

const PacketInfo* PacketListModel::getPacketInfo(uint16_t code)
{
    if(!mPacketInfo.contains(code))
        return 0;

    return mPacketInfo.value(code, 0);
}

int32_t PacketListModel::packetLimit() const
{
    return mPacketLimit;
}

void PacketListModel::setPacketLimit(int32_t limit)
{
    mPacketLimit = limit;

    if(mPacketLimit && mPacketData.count() > mPacketLimit)
    {
        int removeCount = mPacketData.count() - mPacketLimit;

        beginRemoveRows(QModelIndex(), 0, removeCount - 1);

        while(removeCount-- > 0)
            delete mPacketData.takeFirst();

        endRemoveRows();
    }
}
