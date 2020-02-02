/**
 * @file tools/capgrep/src/Packets.cpp
 * @ingroup capgrep
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Clipboard actions for specific packets.
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

#include "Packets.h"

#include "MainWindow.h"
#include "PacketListModel.h"
#include "PacketData.h"

// Qt Libraries
#include <PushIgnore.h>
#include <QStringList>

#include <QClipboard>
#include <QMessageBox>
#include <QApplication>
#include <PopIgnore.h>

#define tr(s) QObject::trUtf8(s)

void action0014(PacketData *d, libcomp::Packet& p,
    libcomp::Packet& packetBefore)
{
    Q_UNUSED(d)
    Q_UNUSED(packetBefore)

    uint32_t uid = p.ReadU32Little();
    uint32_t id = p.ReadU32Little();

    p.Skip(8); // Zone info

    float x = p.ReadFloat();
    float y = p.ReadFloat();
    float rot = p.ReadFloat();

    QString xml = QString("<hNPC id=\"%1\" uid=\"%2\" convo=\"0\" "
        "action=\"0\"\n\tx=\"%3\" y=\"%4\" rot=\"%5\"/>\n").arg(
        id).arg(uid).arg(x).arg(y).arg(rot);

    qApp->clipboard()->setText(xml);
}

void action0015(PacketData *d, libcomp::Packet& p,
    libcomp::Packet& packetBefore)
{
    Q_UNUSED(d)
    Q_UNUSED(packetBefore)

    uint32_t uid = p.ReadU32Little();
    uint32_t id = p.ReadU32Little();
    uint32_t state = p.ReadU8();

    p.Skip(8); // Zone info

    float x = p.ReadFloat();
    float y = p.ReadFloat();
    float rot = p.ReadFloat();

    QString xml = QString("<oNPC id=\"%1\" uid=\"%2\" state=\"%3\" "
        "convo=\"0\" action=\"0\"\n\tx=\"%4\" y=\"%5\" rot=\"%6\"/>").arg(
        id).arg(uid).arg(state).arg(x).arg(y).arg(rot);

    qApp->clipboard()->setText(xml);
}

void action0023(PacketData *d, libcomp::Packet& p,
    libcomp::Packet& packetBefore)
{
    Q_UNUSED(d)
    Q_UNUSED(packetBefore)

    uint32_t zone = p.ReadU32Little();
    uint32_t set = p.ReadU32Little();

    float x = p.ReadFloat();
    float y = p.ReadFloat();
    float rot = p.ReadFloat();

    uint32_t uid = p.ReadU32Little();

    QString xml = QString("<zone>\n\t<info id=\"%1\" set=\"%2\" uid=\"%3\" "
        "global=\"0\" dropZone=\"0\"\n\t\tx=\"%4\" y=\"%5\" rot=\"%6\" "
        "actionOnZoneIn=\"0\" actionOnZoneOut=\"0\"/>\n").arg(
        zone).arg(set).arg(uid).arg(x).arg(y).arg(rot);

    PacketListModel *model = MainWindow::getSingletonPtr()->packetModel();

    QMap<uint32_t, QString> npcs;

    for(int i = 0; i < model->rowCount(); i++)
    {
        PacketData *d2 = model->packetAt(i);

        if(!d2)
            continue;

        if(d2->cmd < 0x0014 || d2->cmd > 0x0015)
            continue;

        libcomp::Packet p2;
        p2.WriteArray(d2->data.constData(), static_cast<uint32_t>(
            d2->data.size()));
        p2.Rewind();

        if(d2->cmd == 0x0014) // hNPC
        {
            uint32_t uid2 = p2.ReadU32Little();
            uint32_t id = p2.ReadU32Little();

            if( npcs.contains(uid2) )
                continue;

            uint32_t zone_set = p2.ReadU32Little();
            uint32_t zone_id = p2.ReadU32Little();

            if(zone_id != zone || zone_set != set)
                continue;

            float x2 = p2.ReadFloat();
            float y2 = p2.ReadFloat();
            float rot2 = p2.ReadFloat();

            npcs[uid2] = QString("\t<hNPC id=\"%1\" uid=\"%2\" convo=\"0\" "
                "action=\"0\"\n\t\tx=\"%3\" y=\"%4\" rot=\"%5\"/>\n").arg(
                id).arg(uid2).arg(x2).arg(y2).arg(rot2);
        }
        else if(d2->cmd == 0x0015) // oNPC
        {
            uint32_t uid2 = p2.ReadU32Little();
            uint32_t id = p2.ReadU32Little();
            uint32_t state = p2.ReadU8();

            if( npcs.contains(uid2) )
                continue;

            uint32_t zone_set = p2.ReadU32Little();
            uint32_t zone_id = p2.ReadU32Little();

            if(zone_id != zone || zone_set != set)
                continue;

            float x2 = p2.ReadFloat();
            float y2 = p2.ReadFloat();
            float rot2 = p2.ReadFloat();

            npcs[uid2] = QString("\t<oNPC id=\"%1\" uid=\"%2\" state=\"%3\" "
                "convo=\"0\" action=\"0\"\n\t\tx=\"%4\" y=\"%5\" "
                "rot=\"%6\"/>\n").arg(id).arg(uid2).arg(state).arg(
                x2).arg(y2).arg(rot2);
        }
    }

    foreach(QString n, npcs)
        xml += n;

    xml += "</zone>\n";

    qApp->clipboard()->setText(xml);
}

void action00A7(PacketData *d, libcomp::Packet& p,
    libcomp::Packet& packetBefore)
{
    Q_UNUSED(d)
    Q_UNUSED(packetBefore)

    p.Skip(4);

    QString xml = QString("<text id=\"%1\"/>").arg( p.ReadU32Little() );

    qApp->clipboard()->setText(xml);
}

void action00AC(PacketData *d, libcomp::Packet& p,
    libcomp::Packet& packetBefore)
{
    Q_UNUSED(d)
    Q_UNUSED(packetBefore)

    p.Skip(4);

    QString xml = QString("<choice id=\"%1\">\n").arg( p.ReadU32Little() );

    uint32_t count = p.ReadU32Little();
    for(uint32_t i = 0; i < count; i++)
    {
        uint32_t key = p.ReadU32Little();
        uint32_t msg = p.ReadU32Little();

        xml += QString("\t<option id=\"%1\" key=\"%2\"/>\n").arg(msg).arg(key);
    }

    xml += "</choice>";

    qApp->clipboard()->setText(xml);
}

void action00B9(PacketData *d, libcomp::Packet& p,
    libcomp::Packet& packetBefore)
{
    Q_UNUSED(d)

    // Sanity check the packet.
    bool valid = p.Size() > 2 && p.Size() == packetBefore.Size();

    // Get the number of bytes in the valuable bitfield.
    uint16_t numBytes = valid ? p.ReadU16Little() : (uint16_t)0;

    // Sanity check the previous bitfield.
    if(valid)
        valid &= numBytes == packetBefore.ReadU16Little();

    // Make sure the number of bytes is correct.
    valid &= numBytes == p.Left();

    // Bail if the packets don't match.
    if(!valid)
    {
        QMessageBox::critical(0, tr("SC_Send_ValuablesFlag"),
            tr("No previous packet or the packet is corrupt."));

        return;
    }

    // Store a list of the changed flags.
    QStringList changedFlags;

    for(uint16_t i = 0; i < numBytes; i++)
    {
        // Get a byte from the bitfield.
        uint8_t current = p.ReadU8();
        uint8_t previous = packetBefore.ReadU8();

        for(int j = 0; j < 8; j++)
        {
            // If the flag changes, mark it.
            if((current & 1) != (previous & 1))
                changedFlags.append(QString::number(i * 8 + j) +
                    ((current & 1) ? " set" : " cleared"));

            // Shift the byte down and move to the next flag.
            current = (uint8_t)(current >> 1);
            previous = (uint8_t)(previous >> 1);
        }
    }

    // Notify the user which flags changed.
    if(changedFlags.isEmpty())
    {
        QMessageBox::information(0, tr("SC_Send_ValuablesFlag"),
            tr("No flags have changed since the previous packet."));
    }
    else
    {
        QMessageBox::information(0, tr("SC_Send_ValuablesFlag"),
            tr("The following flags have changed: %1").arg(
            changedFlags.join(", ")));
    }
}
