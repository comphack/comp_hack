/**
 * @file tools/capfilter/src/ShopFilter.h
 * @ingroup capfilter
 *
 * @author HACKfrost
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

#ifndef TOOLS_CAPFILTER_SRC_SHOPFILTER_H
#define TOOLS_CAPFILTER_SRC_SHOPFILTER_H

// libcomp Includes
#include <DefinitionManager.h>

// C++11 Includes
#include <unordered_map>

// comp_capfilter
#include "CommandFilter.h"

namespace objects
{
class ServerShop;
}

class ShopFilter : public CommandFilter
{
public:
    ShopFilter(const char *szProgram,
        const libcomp::String& dataStorePath);
    virtual ~ShopFilter();

    virtual bool ProcessCommand(const libcomp::String& capturePath,
        uint16_t commandCode, libcomp::ReadOnlyPacket& packet);
    virtual bool PostProcess();

private:
    libcomp::DefinitionManager mDefinitions;
    std::unordered_map<uint32_t,
        std::list<std::shared_ptr<objects::ServerShop>>> mShops;
};

#endif // TOOLS_CAPFILTER_SRC_SHOPFILTER_H
