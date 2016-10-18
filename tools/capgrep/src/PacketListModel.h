/**
 * @file tools/capgrep/src/PacketListModel.h
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

#ifndef TOOLS_CAPGREP_SRC_PACKETLISTMODEL_H
#define TOOLS_CAPGREP_SRC_PACKETLISTMODEL_H

#include <PushIgnore.h>
#include <QAbstractListModel>

#include <QIcon>
#include <PopIgnore.h>

#include <stdint.h>

// Forward Declaration
class PacketData;

class PacketInfo
{
public:
    uint16_t code;
    uint8_t origin;
    QString name, desc;
};

class PacketListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    PacketListModel(QObject *parent = 0);

    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex& index,
        int role = Qt::DisplayRole) const;

    PacketData* packetBefore(int index) const;
    PacketData* packetAt(int index) const;
    QModelIndex modelIndex(int index) const;

    void setPacketData(const QList<PacketData*>& packetData);
    void addPacketData(const QList<PacketData*>& packetData);
    void clear();
    void reset();

    int32_t packetLimit() const;
    void setPacketLimit(int32_t limit);

    static const PacketInfo* getPacketInfo(uint16_t code);

protected:
    static void loadPacketInfo();

    int32_t mPacketLimit;

    QList<QIcon> mIcons;
    QList<PacketData*> mPacketData;

    static QHash<uint16_t, PacketInfo*> mPacketInfo;
};

#endif // TOOLS_CAPGREP_SRC_PACKETLISTMODEL_H
