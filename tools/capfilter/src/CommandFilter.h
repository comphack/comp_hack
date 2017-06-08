/**
 * @file tools/capfilter/src/CommandFilter.h
 * @ingroup capfilter
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Packet filter dialog.
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

#ifndef TOOLS_CAPFILTER_SRC_COMMANDFILTER_H
#define TOOLS_CAPFILTER_SRC_COMMANDFILTER_H

 // libcomp Includes
#include <CString.h>
#include <Packet.h>

class CommandFilter
{
public:
    virtual ~CommandFilter()
    {
    }

    virtual bool ProcessCommand(const libcomp::String& capturePath,
        uint16_t commandCode, libcomp::ReadOnlyPacket& packet) = 0;
    virtual bool PostProcess() = 0;
};


#endif // TOOLS_CAPFILTER_SRC_COMMANDFILTER_H
