/**
 * @file libcomp/src/ServerConstants.h
 * @ingroup libcomp
 *
 * @author HACKfrost
 *
 * @brief Server side configurable constants for logical concepts
 * that match binary file IDs.
 *
 * This file is part of the COMP_hack Library (libcomp).
 *
 * Copyright (C) 2012-2016 COMP_hack Team <compomega@tutanota.com>
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

#ifndef LIBCOMP_SRC_SERVERCONSTANTS_H
#define LIBCOMP_SRC_SERVERCONSTANTS_H

// libcomp Includes
#include "CString.h"

// Standard C++11 Includes
#include <array>
#include <set>
#include <unordered_map>

namespace libcomp
{

/**
 * Static Accessor class for the initalization and retrieval of
 * ServerConstants::Data.
 */
class ServerConstants
{
private:
/**
 * Server side configurable constants data container. Despite being
 * loaded after application level constants typically are, at any given
 * point there will be exactly one of these which can only be accessed
 * as a constant reference.
 */
struct Data
{
    /// Demon ID of elemental type: Flaemis (フレイミーズ)
    uint32_t ELEMENTAL_1_FLAEMIS;

    /// Demon ID of elemental type: Aquans (アクアンズ)
    uint32_t ELEMENTAL_2_AQUANS;

    /// Demon ID of elemental type: Aeros (エアロス)
    uint32_t ELEMENTAL_3_AEROS;

    /// Demon ID of elemental type: Erthys (アーシーズ)
    uint32_t ELEMENTAL_4_ERTHYS;

    /// Demon depo menu event ID
    String EVENT_MENU_DEMON_DEPO;

    /// Homepoint update prompt menu event ID
    String EVENT_MENU_HOMEPOINT;

    /// Item depo menu event ID
    String EVENT_MENU_ITEM_DEPO;

    /// Item ID of item type: Macca (マッカ)
    uint32_t ITEM_MACCA;

    /// Item ID of item type: Macca Note (５００００マッカ紙幣)
    uint32_t ITEM_MACCA_NOTE;

    /// Item ID of item type: Magnetite (マグネタイト)
    uint32_t ITEM_MAGNETITE;

    /// Item ID of item type: Balm of Life (反魂香)
    uint32_t ITEM_BALM_OF_LIFE;

    /// Item ID of the 1 day demon box rental ticket
    uint32_t RENTAL_DEMON_1;

    /// Item ID of the 3 day demon box rental ticket
    uint32_t RENTAL_DEMON_3;

    /// Item ID of the 7 day demon box rental ticket
    uint32_t RENTAL_DEMON_7;

    /// Item ID of the 30 day demon box rental ticket
    uint32_t RENTAL_DEMON_30;

    /// Item ID of the 60 day demon box rental ticket
    uint32_t RENTAL_DEMON_60;

    /// Item ID of the 90 day demon box rental ticket
    uint32_t RENTAL_DEMON_90;

    /// Item ID of the 1 day item box rental ticket
    uint32_t RENTAL_ITEM_1;

    /// Item ID of the 3 day item box rental ticket
    uint32_t RENTAL_ITEM_3;

    /// Item ID of the 7 day item box rental ticket
    uint32_t RENTAL_ITEM_7;

    /// Item ID of the 30 day item box rental ticket
    uint32_t RENTAL_ITEM_30;

    /// Item ID of the 60 day item box rental ticket
    uint32_t RENTAL_ITEM_60;

    /// Item ID of the 90 day item box rental ticket
    uint32_t RENTAL_ITEM_90;

    /// Function ID of clan formation item skills
    uint16_t SKILL_CLAN_FORM;

    /// Function ID of equipment changing skills
    uint16_t SKILL_EQUIP_ITEM;

    /// Function ID of skills that store the demon in the COMP
    uint16_t SKILL_STORE_DEMON;

    /// Function ID of skills that summon a demon from the COMP
    uint16_t SKILL_SUMMON_DEMON;

    /// Function ID of homepoint warp "Traesto" skills
    uint16_t SKILL_TRAESTO;

    /// Status effect ID of summon sync level 1
    uint32_t STATUS_SUMMON_SYNC_1;

    /// Status effect ID of summon sync level 2
    uint32_t STATUS_SUMMON_SYNC_2;

    /// Status effect ID of summon sync level 3
    uint32_t STATUS_SUMMON_SYNC_3;

    /// Default skills to add to a new character
    std::set<uint32_t> DEFAULT_SKILLS;

    /// Map of clan formation item IDs to their corresponding home base zones
    std::unordered_map<uint32_t, uint32_t> CLAN_FORM_MAP;

    /// Array of skill IDs gained at clan levels 1-10
    std::array<std::set<uint32_t>, 10> CLAN_LEVEL_SKILLS;
};

public:
    /**
     * Initialize the server side constants from an XML file
     * @param filePath Path to the constants XML file
     * @return false if anything failed during initialization
     */
    static bool Initialize(const String& filePath);

    /**
     * Get a constant reference to the server side constants
     * data
     * @return Reference to the server side constants data
     */
    static const Data& GetConstants();

private:
    /**
     * Utility function to load a string from the constants read
     * from the XML file
     * @param value Value assigned to a constant
     * @param prop String reference to assign the value to
     * @return true on success, false on failure
     */
    static bool LoadString(const std::string& value, String& prop);

    /**
     * Utility function to load a string from the constants read
     * from the XML file and assign it to an integer
     * @param value Value assigned to a constant
     * @param prop Integer reference to assign the value to
     * @return true on success, false on failure
     */
    template<typename T>
    static bool LoadInteger(const std::string& value, T& prop)
    {
        bool success = false;
        libcomp::String outString;
        if(LoadString(value, outString))
        {
            prop = outString.ToInteger<T>(&success);
        }

        return success;
    }

    /**
     * Utility function to load a string from the constants read
     * from the XML file and assign it to a decimal
     * @param value Value assigned to a constant
     * @param prop Decimal reference to assign the value to
     * @return true on success, false on failure
     */
    template<typename T>
    static bool LoadDecimal(const std::string& value, T& prop)
    {
        bool success = false;
        libcomp::String outString;
        if(LoadString(value, outString))
        {
            prop = outString.ToDecimal<T>(&success);
        }

        return success;
    }

    /// Container for all server side constants
    static Data sConstants;
};

} // namspace libcomp

#define SVR_CONST libcomp::ServerConstants::GetConstants()

#endif // LIBCOMP_SRC_SERVERCONSTANTS_H