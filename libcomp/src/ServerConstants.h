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

#ifndef LIBCOMP_SRC_SERVERCONSTANTS_H
#define LIBCOMP_SRC_SERVERCONSTANTS_H

// libcomp Includes
#include "CString.h"

// Standard C++11 Includes
#include <array>
#include <set>
#include <unordered_map>

namespace tinyxml2
{
class XMLElement;
}

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

    /// COMP shop menu event ID
    String EVENT_MENU_COMP_SHOP;

    /// Demon depo menu event ID
    String EVENT_MENU_DEMON_DEPO;

    /// Item depo menu event ID
    String EVENT_MENU_ITEM_DEPO;

    /// Item ID of item type: Macca (マッカ)
    uint32_t ITEM_MACCA;

    /// Item ID of item type: Macca Note (５００００マッカ紙幣)
    uint32_t ITEM_MACCA_NOTE;

    /// Item ID of item type: Magnetite (マグネタイト)
    uint32_t ITEM_MAGNETITE;

    /// Item ID of item type: Magnetite Presser (ＭＡＧプレッサーα)
    uint32_t ITEM_MAG_PRESSER;

    /// Item ID of item type: Balm of Life (反魂香)
    uint32_t ITEM_BALM_OF_LIFE;

    /// Item ID of item type: Balm of Life (Demon) (反魂香（魔具）)
    uint32_t ITEM_BALM_OF_LIFE_DEMON;

    /// Item ID of item type: Kreuz (クロイツ)
    uint32_t ITEM_KREUZ;

    /// Item ID of item type: Rakutavi's Bloodstone (ラクタヴィの血石)
    uint32_t ITEM_RBLOODSTONE;

    /// Menu ID of the demon fusion (kreuz) process
    uint32_t MENU_FUSION_KZ;

    /// Menu ID of an item repair (kreuz) shop
    uint32_t MENU_REPAIR_KZ;

    /// Menu ID of the Tri-Fusion process
    uint32_t MENU_TRIFUSION;

    /// Menu ID of the Tri-Fusion (solo) process
    uint32_t MENU_TRIFUSION_KZ;

    /// Function ID of "cameo" skills
    uint16_t SKILL_CAMEO;

    /// Function ID of clan formation item skills
    uint16_t SKILL_CLAN_FORM;

    /// Function ID for the demonic compendium add skill
    uint16_t SKILL_DCM;

    /// Function ID of equipment changing skills
    uint16_t SKILL_EQUIP_ITEM;

    /// Function ID skills that edit equipment modifications
    uint16_t SKILL_EQUIP_MOD_EDIT;

    /// Function ID of expertise class down skills
    uint16_t SKILL_EXPERT_CLASS_DOWN;

    /// Function ID of expertise skill forget skills
    uint16_t SKILL_EXPERT_FORGET;

    /// Function ID of expertise all skill forget skills
    uint16_t SKILL_EXPERT_FORGET_ALL;

    /// Function ID of expertise rank down skills
    uint16_t SKILL_EXPERT_RANK_DOWN;

    /// Function ID of familiarity boosting skills
    uint16_t SKILL_FAM_UP;

    /// Function ID of familiarity boosting item skills
    uint16_t SKILL_ITEM_FAM_UP;

    /// Function ID of familiarity lowering "Mooch" skills
    uint16_t SKILL_MOOCH;

    /// Function ID of fixed point max durability increase skills
    uint16_t SKILL_MAX_DURABILITY_FIXED;

    /// Function ID of random range point max durability increase skills
    uint16_t SKILL_MAX_DURABILITY_RANDOM;

    /// Function ID of character skill point reallocation skills
    uint16_t SKILL_RESPEC;

    /// Function ID of rest skills
    uint16_t SKILL_REST;

    /// Function ID of skills that simply execute and send a special
    /// request packet after completion
    uint16_t SKILL_SPECIAL_REQUEST;

    /// Function ID of skills that store the demon in the COMP
    uint16_t SKILL_STORE_DEMON;

    /// Function ID of skills that kill the user as an effect
    uint16_t SKILL_SUICIDE;

    /// Function ID of skills that summon a demon from the COMP
    uint16_t SKILL_SUMMON_DEMON;

    /// Function ID of homepoint warp "Traesto" skills
    uint16_t SKILL_TRAESTO;

    /// Function ID of "Arcadia" warp skills paired with the zone
    /// ID and zone in spot ID
    std::array<uint32_t, 3> SKILL_TRAESTO_ARCADIA;

    /// Function ID of "Kakyojo" warp skills paired with the zone
    /// ID and zone in spot ID
    std::array<uint32_t, 3> SKILL_TRAESTO_KAKYOJO;

    /// Function ID of "Souhonzan" warp skills paired with the zone
    /// ID and zone in spot ID
    std::array<uint32_t, 3> SKILL_TRAESTO_SOUHONZAN;

    /// Function ID of zone targeting warp skills
    uint16_t SKILL_WARP;

    /// Function ID of partner demon granting XP skills
    uint16_t SKILL_XP_PARTNER;

    /// Function ID of self granting XP skills
    uint16_t SKILL_XP_SELF;

    /// Status effect ID of instant death
    uint32_t STATUS_DEATH;

    /// Status effect ID indicating an active demon quest expiration
    uint32_t STATUS_DEMON_QUEST_ACTIVE;

    /// Status effect ID of (demon solo used) entity hide effect
    uint32_t STATUS_HIDDEN;

    /// Status effect ID of summon sync level 1
    uint32_t STATUS_SUMMON_SYNC_1;

    /// Status effect ID of summon sync level 2
    uint32_t STATUS_SUMMON_SYNC_2;

    /// Status effect ID of summon sync level 3
    uint32_t STATUS_SUMMON_SYNC_3;

    /// Valuable ID of the demonic compendium V1
    uint16_t VALUABLE_DEVIL_BOOK_V1;

    /// Valuable ID of the demonic compendium V2
    uint16_t VALUABLE_DEVIL_BOOK_V2;

    /// Valuable ID of the material tank that stores disassembled items
    uint16_t VALUABLE_MATERIAL_TANK;

    /// Default zone to move players when no other zone is found
    uint32_t ZONE_DEFAULT;

    /// Map of cameo item IDs to transformation status effect IDs, if
    /// more than one status effect is listed, a random one is chosen
    std::unordered_map<uint32_t, std::list<uint32_t>> CAMEO_MAP;

    /// Map of clan formation item IDs to their corresponding home base zones
    std::unordered_map<uint32_t, uint32_t> CLAN_FORM_MAP;

    /// Array of skill IDs gained at clan levels 1-10
    std::array<std::set<uint32_t>, 10> CLAN_LEVEL_SKILLS;

    /// Map of the number of entries in the compendium required to gain the
    /// specified tokusei IDs
    std::unordered_map<uint16_t, std::set<int32_t>> DEMON_BOOK_BONUS;

    /// Map of demon crystal item types to races that can be fused with them
    /// for cyrstallization
    std::unordered_map<uint32_t, std::set<uint8_t>> DEMON_CRYSTALS;

    /// List of bonus XP gained from sequential demon quest completions
    std::list<uint32_t> DEMON_QUEST_XP;

    /// Item IDs of demon box rental tickets to their corresponding day lengths
    std::unordered_map<uint32_t, uint32_t> DEPO_MAP_DEMON;

    /// Item IDs of item box rental tickets to their corresponding day lengths
    std::unordered_map<uint32_t, uint32_t> DEPO_MAP_ITEM;

    /// Item IDs with parameters used for the EQUIP_MOD_EDIT function ID skill
    std::unordered_map<uint32_t, std::array<int32_t, 3>> EQUIP_MOD_EDIT_ITEMS;

    /// Passive fusion skill IDs to result race filters and success boosts
    std::unordered_map<uint32_t, std::array<int8_t, 2>> FUSION_BOOST_SKILLS;

    /// Fusion status effect IDs to success boosts
    std::unordered_map<uint32_t, uint8_t> FUSION_BOOST_STATUSES;

    /// Map of the number of completed quests required to gain the
    /// specified tokusei IDs
    std::unordered_map<uint16_t, int32_t> QUEST_BONUS;

    /// Array of item IDs used for special functions, indexed in the same order
    /// as the RateScaling fields on the following objects:
    /// Index 0) MiDisassemblyTriggerData
    /// Index 1) MiModificationTriggerData
    /// Index 2) MiModificationExtEffectData
    /// Index 3) MiSynthesisData
    std::array<std::list<uint32_t>, 4> RATE_SCALING_ITEMS;

    /// Item IDs mapped to success, great success boosts. Items can be equipped
    /// or be part of the fusion.
    std::unordered_map<uint32_t, std::array<uint8_t, 2>> SPIRIT_FUSION_BOOST;

    /// Item/skill IDs with parameters used for synth calculations
    std::unordered_map<uint32_t, std::array<int32_t, 3>> SYNTH_ADJUSTMENTS;

    /// Synth skill IDs for demon crystallization, tarot enchant, soul enchant,
    /// synth melee and synth gun skills
    std::array<uint32_t, 5> SYNTH_SKILLS;

    /// Level ranges to use for TriFusion of 3 "dark" family demons
    std::list<std::pair<uint8_t, uint32_t>> TRIFUSION_SPECIAL_DARK;

    /// Set of dual elemental TriFusion special results listed by the two
    /// involved elemental types, then up to 3 valid races for the third demon
    /// and ending with the resulting demon type
    std::array<std::array<uint32_t, 6>, 6> TRIFUSION_SPECIAL_ELEMENTAL;

    /// Item IDs that allow creation of VA items from a normal one
    std::set<uint32_t> VA_ADD_ITEM;
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
     * Utility function to load a list of strings from the constants read
     * from the XML file
     * @param elem Pointer to the string list parent element
     * @param prop List of strings reference to assign the value to
     * @return true on success, false on failure
     */
    static bool LoadStringList(const tinyxml2::XMLElement* elem,
        std::list<String>& prop);

    /**
     * Utility function to load a list of strings into an array
     * @param prop Array of integers to assign the value to
     * @param values List of strings reference to pull the values from
     * @return true on success, false on failure
     */
    template<typename T, std::size_t SIZE>
    static bool ToIntegerArray(std::array<T, SIZE>& prop,
        std::list<String> values)
    {
        if(SIZE != values.size())
        {
            return false;
        }

        bool success;
        size_t idx = 0;
        for(auto str : values)
        {
            T val = str.ToInteger<T>(&success);
            if(!success)
            {
                return false;
            }

            prop[idx++] = val;
        }

        return true;
    }

    /**
     * Utility function to load key value string pairs from the
     * constants read from the XML file
     * @param elem Pointer to the pair list parent element
     * @param map String to string map to assign the value to
     * @return true on success, false on failure
     */
    static bool LoadKeyValueStrings(const tinyxml2::XMLElement* elem,
        std::unordered_map<std::string, std::string>& map);

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

    /**
     * Utility function to load a map of string to string pairs from
     * the constants read from the XML file and convert to integer types
     * @param valueMap Value map assigned to a constant
     * @param propMap Integer map reference to assign the values to
     * @return true on success, false on failure
     */
    template<typename K, typename V>
    static bool LoadIntegerMap(const std::unordered_map<std::string,
        std::string>& valueMap, std::unordered_map<K, V>& propMap)
    {
        for(auto pair : valueMap)
        {
            K key = 0;
            V val = 0;
            if(LoadInteger(pair.first, key) && LoadInteger(pair.second, val))
            {
                propMap[key] = val;
            }
            else
            {
                return false;
            }
        }

        return true;
    }

    /**
     * Utility function to load convert a string to a list of numeric
     * ranges and return in a list.
     * @param value String of numeric ranges
     * @param success Output success indicator
     * @return List of converted numbers or empty if failure occurs
     */
    template<typename T>
    static std::list<T> ToIntegerRange(const std::string& value, bool& success)
    {
        std::list<T> results;

        auto params = libcomp::String(value).Split(",");
        for(auto param : params)
        {
            auto subParams = libcomp::String(param).Split("-");
            if(subParams.size() == 2)
            {
                // Min-max
                T min = 0;
                T max = 0;
                if(LoadInteger(subParams.front().C(), min) &&
                    LoadInteger(subParams.back().C(), max) &&
                    min < max)
                {
                    for(T i = min; i <= max; i++)
                    {
                        results.push_back(i);
                    }
                }
                else
                {
                    success = false;
                }
            }
            else if(subParams.size() == 1)
            {
                // Single value
                T p = 0;
                if(LoadInteger(param.C(), p))
                {
                    results.push_back(p);
                }
                else
                {
                    success = false;
                }
            }
            else
            {
                success = false;
            }

            if(!success)
            {
                results.clear();
                return results;
            }
        }

        return results;
    }

    /// Container for all server side constants
    static Data sConstants;
};

} // namspace libcomp

#define SVR_CONST libcomp::ServerConstants::GetConstants()

#endif // LIBCOMP_SRC_SERVERCONSTANTS_H
