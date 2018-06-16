/**
 * @file libcomp/src/ServerConstants.cpp
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

#include "ServerConstants.h"

// Standard C++ Includes
#include <unordered_map>

// tinyxml2 Includes
#include <PushIgnore.h>
#include <tinyxml2.h>
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>

using namespace libcomp;

ServerConstants::Data ServerConstants::sConstants = {};

bool ServerConstants::Initialize(const String& filePath)
{
    tinyxml2::XMLDocument doc;
    if(tinyxml2::XML_SUCCESS != doc.LoadFile(filePath.C()))
    {
        LOG_ERROR("Server contants XML is not valid.");
        return false;
    }

    // Read all constants as strings
    std::unordered_map<std::string, std::string> constants;
    std::unordered_map<std::string, const tinyxml2::XMLElement*> complexConstants;
    const tinyxml2::XMLElement *pMember = doc.RootElement()->
        FirstChildElement("constant");
    while(pMember)
    {
        const char *szName = pMember->Attribute("name");

        if(szName && pMember->FirstChild())
        {
            const tinyxml2::XMLElement *child = pMember->FirstChildElement();
            if(child)
            {
                complexConstants[szName] = child;
            }
            else
            {
                const tinyxml2::XMLText *pText = pMember->FirstChild()->ToText();
                if(pText)
                {
                    constants[szName] = pText->Value();
                }
            }
        }

        pMember = pMember->NextSiblingElement("constant");
    }

    // Convert and assign all entries
    bool success = true;

    // Load event constants
    success &= LoadString(constants["EVENT_MENU_COMP_SHOP"],
        sConstants.EVENT_MENU_COMP_SHOP);
    success &= LoadString(constants["EVENT_MENU_DEMON_DEPO"],
        sConstants.EVENT_MENU_DEMON_DEPO);
    success &= LoadString(constants["EVENT_MENU_ITEM_DEPO"],
        sConstants.EVENT_MENU_ITEM_DEPO);

    // Load demon constants
    success &= LoadInteger(constants["ELEMENTAL_1_FLAEMIS"],
        sConstants.ELEMENTAL_1_FLAEMIS);
    success &= LoadInteger(constants["ELEMENTAL_2_AQUANS"],
        sConstants.ELEMENTAL_2_AQUANS);
    success &= LoadInteger(constants["ELEMENTAL_3_AEROS"],
        sConstants.ELEMENTAL_3_AEROS);
    success &= LoadInteger(constants["ELEMENTAL_4_ERTHYS"],
        sConstants.ELEMENTAL_4_ERTHYS);

    // Load item constants
    success &= LoadInteger(constants["ITEM_MACCA"],
        sConstants.ITEM_MACCA);
    success &= LoadInteger(constants["ITEM_MACCA_NOTE"],
        sConstants.ITEM_MACCA_NOTE);
    success &= LoadInteger(constants["ITEM_MAGNETITE"],
        sConstants.ITEM_MAGNETITE);
    success &= LoadInteger(constants["ITEM_MAG_PRESSER"],
        sConstants.ITEM_MAG_PRESSER);
    success &= LoadInteger(constants["ITEM_BALM_OF_LIFE"],
        sConstants.ITEM_BALM_OF_LIFE);
    success &= LoadInteger(constants["ITEM_BALM_OF_LIFE_DEMON"],
        sConstants.ITEM_BALM_OF_LIFE_DEMON);
    success &= LoadInteger(constants["ITEM_KREUZ"],
        sConstants.ITEM_KREUZ);
    success &= LoadInteger(constants["ITEM_RBLOODSTONE"],
        sConstants.ITEM_RBLOODSTONE);

    // Load menu constants
    success &= LoadInteger(constants["MENU_FUSION_KZ"],
        sConstants.MENU_FUSION_KZ);
    success &= LoadInteger(constants["MENU_REPAIR_KZ"],
        sConstants.MENU_REPAIR_KZ);
    success &= LoadInteger(constants["MENU_TRIFUSION"],
        sConstants.MENU_TRIFUSION);
    success &= LoadInteger(constants["MENU_TRIFUSION_KZ"],
        sConstants.MENU_TRIFUSION_KZ);

    // Load skill constants
    success &= LoadInteger(constants["SKILL_ABS_DAMAGE"],
        sConstants.SKILL_ABS_DAMAGE);
    success &= LoadInteger(constants["SKILL_BOSS_SPECIAL"],
        sConstants.SKILL_BOSS_SPECIAL);
    success &= LoadInteger(constants["SKILL_CAMEO"],
        sConstants.SKILL_CAMEO);
    success &= LoadInteger(constants["SKILL_CLAN_FORM"],
        sConstants.SKILL_CLAN_FORM);
    success &= LoadInteger(constants["SKILL_CLOAK"],
        sConstants.SKILL_CLOAK);
    success &= LoadInteger(constants["SKILL_GENDER_RESTRICTED"],
        sConstants.SKILL_GENDER_RESTRICTED);
    success &= LoadInteger(constants["SKILL_CULTIVATE_SLOT_UP"],
        sConstants.SKILL_CULTIVATE_SLOT_UP);
    success &= LoadInteger(constants["SKILL_CULTIVATE_UP"],
        sConstants.SKILL_CULTIVATE_UP);
    success &= LoadInteger(constants["SKILL_DCM"],
        sConstants.SKILL_DCM);
    success &= LoadInteger(constants["SKILL_DEMON_FUSION"],
        sConstants.SKILL_DEMON_FUSION);
    success &= LoadInteger(constants["SKILL_DEMON_FUSION_EXECUTE"],
        sConstants.SKILL_DEMON_FUSION_EXECUTE);
    success &= LoadInteger(constants["SKILL_DESPAWN"],
        sConstants.SKILL_DESPAWN);
    success &= LoadInteger(constants["SKILL_DESUMMON"],
        sConstants.SKILL_DESUMMON);
    success &= LoadInteger(constants["SKILL_DIASPORA_QUAKE"],
        sConstants.SKILL_DIASPORA_QUAKE);
    success &= LoadInteger(constants["SKILL_DIGITALIZE"],
        sConstants.SKILL_DIGITALIZE);
    success &= LoadInteger(constants["SKILL_DIGITALIZE_BREAK"],
        sConstants.SKILL_DIGITALIZE_BREAK);
    success &= LoadInteger(constants["SKILL_DIGITALIZE_CANCEL"],
        sConstants.SKILL_DIGITALIZE_CANCEL);
    success &= LoadInteger(constants["SKILL_DURABILITY_DOWN"],
        sConstants.SKILL_DURABILITY_DOWN);
    success &= LoadInteger(constants["SKILL_EQUIP_ITEM"],
        sConstants.SKILL_EQUIP_ITEM);
    success &= LoadInteger(constants["SKILL_EQUIP_MOD_EDIT"],
        sConstants.SKILL_EQUIP_MOD_EDIT);
    success &= LoadInteger(constants["SKILL_ESTOMA"],
        sConstants.SKILL_ESTOMA);
    success &= LoadInteger(constants["SKILL_EXPERT_CLASS_DOWN"],
        sConstants.SKILL_EXPERT_CLASS_DOWN);
    success &= LoadInteger(constants["SKILL_EXPERT_FORGET"],
        sConstants.SKILL_EXPERT_FORGET);
    success &= LoadInteger(constants["SKILL_EXPERT_FORGET_ALL"],
        sConstants.SKILL_EXPERT_FORGET_ALL);
    success &= LoadInteger(constants["SKILL_EXPERT_RANK_DOWN"],
        sConstants.SKILL_EXPERT_RANK_DOWN);
    success &= LoadInteger(constants["SKILL_FAM_UP"],
        sConstants.SKILL_FAM_UP);
    success &= LoadInteger(constants["SKILL_GEM_COST"],
        sConstants.SKILL_GEM_COST);
    success &= LoadInteger(constants["SKILL_HP_DEPENDENT"],
        sConstants.SKILL_HP_DEPENDENT);
    success &= LoadInteger(constants["SKILL_HP_MP_MIN"],
        sConstants.SKILL_HP_MP_MIN);
    success &= LoadInteger(constants["SKILL_ITEM_FAM_UP"],
        sConstants.SKILL_ITEM_FAM_UP);
    success &= LoadInteger(constants["SKILL_LIBERAMA"],
        sConstants.SKILL_LIBERAMA);
    success &= LoadInteger(constants["SKILL_LNC_DAMAGE"],
        sConstants.SKILL_LNC_DAMAGE);
    success &= LoadInteger(constants["SKILL_MAX_DURABILITY_FIXED"],
        sConstants.SKILL_MAX_DURABILITY_FIXED);
    success &= LoadInteger(constants["SKILL_MAX_DURABILITY_RANDOM"],
        sConstants.SKILL_MAX_DURABILITY_RANDOM);
    success &= LoadInteger(constants["SKILL_MINION_DESPAWN"],
        sConstants.SKILL_MINION_DESPAWN);
    success &= LoadInteger(constants["SKILL_MINION_SPAWN"],
        sConstants.SKILL_MINION_SPAWN);
    success &= LoadInteger(constants["SKILL_MOOCH"],
        sConstants.SKILL_MOOCH);
    success &= LoadInteger(constants["SKILL_MOUNT"],
        sConstants.SKILL_MOUNT);
    success &= LoadInteger(constants["SKILL_PIERCE"],
        sConstants.SKILL_PIERCE);
    success &= LoadInteger(constants["SKILL_RANDOM_ITEM"],
        sConstants.SKILL_RANDOM_ITEM);
    success &= LoadInteger(constants["SKILL_RANDOMIZE"],
        sConstants.SKILL_RANDOMIZE);
    success &= LoadInteger(constants["SKILL_RESPEC"],
        sConstants.SKILL_RESPEC);
    success &= LoadInteger(constants["SKILL_REST"],
        sConstants.SKILL_REST);
    success &= LoadInteger(constants["SKILL_SLEEP_RESTRICTED"],
        sConstants.SKILL_SLEEP_RESTRICTED);
    success &= LoadInteger(constants["SKILL_SPAWN"],
        sConstants.SKILL_SPAWN);
    success &= LoadInteger(constants["SKILL_SPAWN_ZONE"],
        sConstants.SKILL_SPAWN_ZONE);
    success &= LoadInteger(constants["SKILL_SPECIAL_REQUEST"],
        sConstants.SKILL_SPECIAL_REQUEST);
    success &= LoadInteger(constants["SKILL_STAT_SUM_DAMAGE"],
        sConstants.SKILL_STAT_SUM_DAMAGE);
    success &= LoadInteger(constants["SKILL_STATUS_DIRECT"],
        sConstants.SKILL_STATUS_DIRECT);
    success &= LoadInteger(constants["SKILL_STATUS_LIMITED"],
        sConstants.SKILL_STATUS_LIMITED);
    success &= LoadInteger(constants["SKILL_STATUS_RANDOM"],
        sConstants.SKILL_STATUS_RANDOM);
    success &= LoadInteger(constants["SKILL_STATUS_RANDOM2"],
        sConstants.SKILL_STATUS_RANDOM2);
    success &= LoadInteger(constants["SKILL_STATUS_RESTRICTED"],
        sConstants.SKILL_STATUS_RESTRICTED);
    success &= LoadInteger(constants["SKILL_STATUS_SCALE"],
        sConstants.SKILL_STATUS_SCALE);
    success &= LoadInteger(constants["SKILL_STORE_DEMON"],
        sConstants.SKILL_STORE_DEMON);
    success &= LoadInteger(constants["SKILL_SUICIDE"],
        sConstants.SKILL_SUICIDE);
    success &= LoadInteger(constants["SKILL_SUMMON_DEMON"],
        sConstants.SKILL_SUMMON_DEMON);
    success &= LoadInteger(constants["SKILL_TAUNT"],
        sConstants.SKILL_TAUNT);
    success &= LoadInteger(constants["SKILL_TRAESTO"],
        sConstants.SKILL_TRAESTO);
    success &= LoadInteger(constants["SKILL_WARP"],
        sConstants.SKILL_WARP);
    success &= LoadInteger(constants["SKILL_XP_PARTNER"],
        sConstants.SKILL_XP_PARTNER);
    success &= LoadInteger(constants["SKILL_XP_SELF"],
        sConstants.SKILL_XP_SELF);
    success &= LoadInteger(constants["SKILL_ZONE_RESTRICTED"],
        sConstants.SKILL_ZONE_RESTRICTED);
    success &= LoadInteger(constants["SKILL_ZONE_RESTRICTED_ITEM"],
        sConstants.SKILL_ZONE_RESTRICTED_ITEM);
    success &= LoadInteger(constants["SKILL_ZONE_TARGET_ALL"],
        sConstants.SKILL_ZONE_TARGET_ALL);

    // Load status effect constants
    success &= LoadInteger(constants["STATUS_BIKE"],
        sConstants.STATUS_BIKE);
    success &= LoadInteger(constants["STATUS_CLOAK"],
        sConstants.STATUS_CLOAK);
    success &= LoadInteger(constants["STATUS_DEATH"],
        sConstants.STATUS_DEATH);
    success &= LoadInteger(constants["STATUS_DEMON_QUEST_ACTIVE"],
        sConstants.STATUS_DEMON_QUEST_ACTIVE);
    success &= LoadInteger(constants["STATUS_HIDDEN"],
        sConstants.STATUS_HIDDEN);
    success &= LoadInteger(constants["STATUS_MOUNT"],
        sConstants.STATUS_MOUNT);
    success &= LoadInteger(constants["STATUS_MOUNT_SUPER"],
        sConstants.STATUS_MOUNT_SUPER);
    success &= LoadInteger(constants["STATUS_SLEEP"],
        sConstants.STATUS_SLEEP);
    success &= LoadInteger(constants["STATUS_SUMMON_SYNC_1"],
        sConstants.STATUS_SUMMON_SYNC_1);
    success &= LoadInteger(constants["STATUS_SUMMON_SYNC_2"],
        sConstants.STATUS_SUMMON_SYNC_2);
    success &= LoadInteger(constants["STATUS_SUMMON_SYNC_3"],
        sConstants.STATUS_SUMMON_SYNC_3);

    // Load (detached) tokusei constants
    success &= LoadInteger(constants["TOKUSEI_BIKE_BOOST"],
        sConstants.TOKUSEI_BIKE_BOOST);

    // Load valuable constants
    success &= LoadInteger(constants["VALUABLE_DEVIL_BOOK_V1"],
        sConstants.VALUABLE_DEVIL_BOOK_V1);
    success &= LoadInteger(constants["VALUABLE_DEVIL_BOOK_V2"],
        sConstants.VALUABLE_DEVIL_BOOK_V2);
    success &= LoadInteger(constants["VALUABLE_DEMON_FORCE"],
        sConstants.VALUABLE_DEMON_FORCE);
    success &= LoadInteger(constants["VALUABLE_FUSION_GAUGE"],
        sConstants.VALUABLE_FUSION_GAUGE);
    success &= LoadInteger(constants["VALUABLE_MATERIAL_TANK"],
        sConstants.VALUABLE_MATERIAL_TANK);

    // Load zone constants
    success &= LoadInteger(constants["ZONE_DEFAULT"],
        sConstants.ZONE_DEFAULT);

    String listStr;
    success &= LoadString(constants["SKILL_TRAESTO_ARCADIA"], listStr) &&
        ToIntegerArray(sConstants.SKILL_TRAESTO_ARCADIA, listStr.Split(","));
    success &= LoadString(constants["SKILL_TRAESTO_DSHINJUKU"], listStr) &&
        ToIntegerArray(sConstants.SKILL_TRAESTO_DSHINJUKU, listStr.Split(","));
    success &= LoadString(constants["SKILL_TRAESTO_KAKYOJO"], listStr) &&
        ToIntegerArray(sConstants.SKILL_TRAESTO_KAKYOJO, listStr.Split(","));
    success &= LoadString(constants["SKILL_TRAESTO_NAKANO_BDOMAIN"], listStr) &&
        ToIntegerArray(sConstants.SKILL_TRAESTO_NAKANO_BDOMAIN, listStr.Split(","));
    success &= LoadString(constants["SKILL_TRAESTO_SOUHONZAN"], listStr) &&
        ToIntegerArray(sConstants.SKILL_TRAESTO_SOUHONZAN, listStr.Split(","));

    auto complexIter = complexConstants.find("CAMEO_MAP");
    if(success && complexIter != complexConstants.end())
    {
        std::unordered_map<std::string, std::string> map;
        success = LoadKeyValueStrings(complexIter->second, map);
        for(auto pair : map)
        {
            uint16_t key;
            if(!LoadInteger(pair.first, key))
            {
                LOG_ERROR("Failed to load CAMEO_MAP key\n");
                success = false;
            }
            else if(sConstants.CAMEO_MAP.find(key) !=
                sConstants.CAMEO_MAP.end())
            {
                LOG_ERROR("Duplicate CAMEO_MAP key encountered\n");
                success = false;
            }
            else
            {
                if(!pair.second.empty())
                {
                    for(uint32_t p : ToIntegerRange<uint32_t>(pair.second,
                        success))
                    {
                        sConstants.CAMEO_MAP[key].push_back(p);
                    }

                    if(!success)
                    {
                        LOG_ERROR("Failed to load an element in CAMEO_MAP\n");
                        break;
                    }
                }
            }

            if(!success)
            {
                break;
            }
        }
    }
    else
    {
        LOG_ERROR("CAMEO_MAP not found\n");
        success = false;
    }

    complexIter = complexConstants.find("CLAN_FORM_MAP");
    if(success && complexIter != complexConstants.end())
    {
        std::unordered_map<std::string, std::string> map;
        success = LoadKeyValueStrings(complexIter->second, map) &&
            LoadIntegerMap(map, sConstants.CLAN_FORM_MAP);
    }
    else
    {
        LOG_ERROR("CLAN_FORM_MAP not found\n");
        success = false;
    }

    complexIter = complexConstants.find("CLAN_LEVEL_SKILLS");
    if(success && complexIter != complexConstants.end())
    {
        std::list<String> strList;
        if(!LoadStringList(complexIter->second, strList))
        {
            LOG_ERROR("Failed to load CLAN_LEVEL_SKILLS\n");
            success = false;
        }
        else
        {
            if(strList.size() != 10)
            {
                LOG_ERROR("CLAN_LEVEL_SKILLS must specify skills for all"
                    " 10 levels\n");
                success = false;
            }
            else
            {
                size_t idx = 0;
                for(auto elemStr : strList)
                {
                    if(!elemStr.IsEmpty())
                    {
                        for(uint32_t p : ToIntegerRange<uint32_t>(elemStr.C(),
                            success))
                        {
                            sConstants.CLAN_LEVEL_SKILLS[idx].insert(p);
                        }

                        if(!success)
                        {
                            LOG_ERROR("Failed to load an element in"
                                " CLAN_LEVEL_SKILLS\n");
                            break;
                        }
                    }

                    idx++;
                }
            }
        }
    }
    else
    {
        LOG_ERROR("CLAN_LEVEL_SKILLS not found\n");
        success = false;
    }

    complexIter = complexConstants.find("DEMON_BOOK_BONUS");
    if(success && complexIter != complexConstants.end())
    {
        std::unordered_map<std::string, std::string> map;
        success = LoadKeyValueStrings(complexIter->second, map);
        for(auto pair : map)
        {
            uint16_t key;
            if(!LoadInteger(pair.first, key))
            {
                LOG_ERROR("Failed to load DEMON_BOOK_BONUS key\n");
                success = false;
            }
            else if(sConstants.DEMON_BOOK_BONUS.find(key) !=
                sConstants.DEMON_BOOK_BONUS.end())
            {
                LOG_ERROR("Duplicate DEMON_BOOK_BONUS key encountered\n");
                success = false;
            }
            else
            {
                if(!pair.second.empty())
                {
                    for(int32_t p : ToIntegerRange<int32_t>(pair.second,
                        success))
                    {
                        sConstants.DEMON_BOOK_BONUS[key].insert(p);
                    }

                    if(!success)
                    {
                        LOG_ERROR("Failed to load an element in"
                            " DEMON_BOOK_BONUS\n");
                        break;
                    }
                }
            }

            if(!success)
            {
                break;
            }
        }
    }
    else
    {
        LOG_ERROR("DEMON_BOOK_BONUS not found\n");
        success = false;
    }

    complexIter = complexConstants.find("DEMON_CRYSTALS");
    if(success && complexIter != complexConstants.end())
    {
        std::unordered_map<std::string, std::string> map;
        success = LoadKeyValueStrings(complexIter->second, map);
        for(auto pair : map)
        {
            uint32_t key;
            if(!LoadInteger(pair.first, key))
            {
                LOG_ERROR("Failed to load DEMON_CRYSTALS key\n");
                success = false;
            }
            else if(sConstants.DEMON_CRYSTALS.find(key) !=
                sConstants.DEMON_CRYSTALS.end())
            {
                LOG_ERROR("Duplicate DEMON_CRYSTALS key encountered\n");
                success = false;
            }
            else
            {
                if(!pair.second.empty())
                {
                    for(uint8_t p : ToIntegerRange<uint8_t>(pair.second,
                        success))
                    {
                        sConstants.DEMON_CRYSTALS[key].insert(p);
                    }

                    if(!success)
                    {
                        LOG_ERROR("Failed to load an element in"
                            " DEMON_CRYSTALS\n");
                        break;
                    }
                }
            }

            if(!success)
            {
                break;
            }
        }
    }
    else
    {
        LOG_ERROR("DEMON_CRYSTALS not found\n");
        success = false;
    }

    complexIter = complexConstants.find("DEMON_FUSION_SKILLS");
    if(success && complexIter != complexConstants.end())
    {
        std::list<String> strList;
        if(!LoadStringList(complexIter->second, strList))
        {
            LOG_ERROR("Failed to load DEMON_FUSION_SKILLS\n");
            success = false;
        }
        else
        {
            if(strList.size() != 21)
            {
                LOG_ERROR("DEMON_FUSION_SKILLS must specify all 21"
                    " inheritance type skill mappings\n");
                success = false;
            }
            else
            {
                size_t idx = 0;
                for(auto elem : strList)
                {
                    auto vals = ToIntegerRange<uint32_t>(elem.C(), success);
                    if(vals.size() != 3)
                    {
                        LOG_ERROR("DEMON_FUSION_SKILLS element encountered"
                            " with level count other than 3\n");
                        success = false;
                        break;
                    }

                    size_t subIdx = 0;
                    for(uint32_t val : vals)
                    {
                        sConstants.DEMON_FUSION_SKILLS[idx][subIdx++] = val;
                    }

                    idx++;
                }
            }
        }
    }
    else
    {
        LOG_ERROR("DEMON_FUSION_SKILLS not found\n");
        success = false;
    }

    complexIter = complexConstants.find("DEMON_QUEST_XP");
    if(success && complexIter != complexConstants.end())
    {
        std::list<String> strList;
        if(!LoadStringList(complexIter->second, strList))
        {
            LOG_ERROR("Failed to load DEMON_QUEST_XP\n");
            success = false;
        }
        else
        {
            for(auto elem : strList)
            {
                uint32_t xp = 0;
                if(LoadInteger(elem.C(), xp))
                {
                    sConstants.DEMON_QUEST_XP.push_back(xp);
                }
                else
                {
                    LOG_ERROR("Failed to load an entry in"
                        " DEMON_QUEST_XP\n");
                    success = false;
                    break;
                }
            }
        }
    }
    else
    {
        LOG_ERROR("DEMON_QUEST_XP not found\n");
        success = false;
    }

    complexIter = complexConstants.find("DEPO_MAP_DEMON");
    if(success && complexIter != complexConstants.end())
    {
        std::unordered_map<std::string, std::string> map;
        success = LoadKeyValueStrings(complexIter->second, map) &&
            LoadIntegerMap(map, sConstants.DEPO_MAP_DEMON);
    }
    else
    {
        LOG_ERROR("DEPO_MAP_DEMON not found\n");
        success = false;
    }

    complexIter = complexConstants.find("DEPO_MAP_ITEM");
    if(success && complexIter != complexConstants.end())
    {
        std::unordered_map<std::string, std::string> map;
        success = LoadKeyValueStrings(complexIter->second, map) &&
            LoadIntegerMap(map, sConstants.DEPO_MAP_ITEM);
    }
    else
    {
        LOG_ERROR("DEPO_MAP_ITEM not found\n");
        success = false;
    }

    complexIter = complexConstants.find("EQUIP_MOD_EDIT_ITEMS");
    if(success && complexIter != complexConstants.end())
    {
        std::unordered_map<std::string, std::string> map;
        success = LoadKeyValueStrings(complexIter->second, map);
        for(auto pair : map)
        {
            uint32_t key;
            if(!LoadInteger(pair.first, key))
            {
                LOG_ERROR("Failed to load EQUIP_MOD_EDIT_ITEMS key\n");
                success = false;
            }
            else if(sConstants.EQUIP_MOD_EDIT_ITEMS.find(key) !=
                sConstants.EQUIP_MOD_EDIT_ITEMS.end())
            {
                LOG_ERROR("Duplicate EQUIP_MOD_EDIT_ITEMS key encountered\n");
                success = false;
            }
            else
            {
                success &= ToIntegerArray(sConstants.EQUIP_MOD_EDIT_ITEMS[key],
                    libcomp::String(pair.second).Split(","));
            }

            if(!success)
            {
                break;
            }
        }
    }
    else
    {
        LOG_ERROR("EQUIP_MOD_EDIT_ITEMS not found\n");
        success = false;
    }

    complexIter = complexConstants.find("FUSION_BOOST_SKILLS");
    if(success && complexIter != complexConstants.end())
    {
        std::unordered_map<std::string, std::string> map;
        success = LoadKeyValueStrings(complexIter->second, map);
        for(auto pair : map)
        {
            uint32_t key;
            if(!LoadInteger(pair.first, key))
            {
                LOG_ERROR("Failed to load FUSION_BOOST_SKILLS key\n");
                success = false;
            }
            else if(sConstants.FUSION_BOOST_SKILLS.find(key) !=
                sConstants.FUSION_BOOST_SKILLS.end())
            {
                LOG_ERROR("Duplicate FUSION_BOOST_SKILLS key encountered\n");
                success = false;
            }
            else
            {
                success &= ToIntegerArray(sConstants.FUSION_BOOST_SKILLS[key],
                    libcomp::String(pair.second).Split(","));
            }

            if(!success)
            {
                break;
            }
        }
    }
    else
    {
        LOG_ERROR("FUSION_BOOST_SKILLS not found\n");
        success = false;
    }

    complexIter = complexConstants.find("FUSION_BOOST_STATUSES");
    if(success && complexIter != complexConstants.end())
    {
        std::unordered_map<std::string, std::string> map;
        success = LoadKeyValueStrings(complexIter->second, map) &&
            LoadIntegerMap(map, sConstants.FUSION_BOOST_STATUSES);
    }
    else
    {
        LOG_ERROR("FUSION_BOOST_STATUSES not found\n");
        success = false;
    }

    complexIter = complexConstants.find("QUEST_BONUS");
    if(success && complexIter != complexConstants.end())
    {
        std::unordered_map<std::string, std::string> map;
        success = LoadKeyValueStrings(complexIter->second, map) &&
            LoadIntegerMap(map, sConstants.QUEST_BONUS);
    }
    else
    {
        LOG_ERROR("QUEST_BONUS not found\n");
        success = false;
    }

    complexIter = complexConstants.find("RATE_SCALING_ITEMS");
    if(success && complexIter != complexConstants.end())
    {
        std::list<String> strList;
        if(!LoadStringList(complexIter->second, strList))
        {
            LOG_ERROR("Failed to load RATE_SCALING_ITEMS\n");
            success = false;
        }
        else
        {
            if(strList.size() != 4)
            {
                LOG_ERROR("RATE_SCALING_ITEMS must specify items for each"
                    " of the 4 types\n");
                success = false;
            }
            else
            {
                size_t idx = 0;
                for(auto elemStr : strList)
                {
                    if(!elemStr.IsEmpty())
                    {
                        for(uint32_t p : ToIntegerRange<uint32_t>(elemStr.C(),
                            success))
                        {
                            sConstants.RATE_SCALING_ITEMS[idx].push_back(p);
                        }

                        if(!success)
                        {
                            LOG_ERROR("Failed to load an element in"
                                " RATE_SCALING_ITEMS\n");
                            break;
                        }
                    }

                    idx++;
                }
            }
        }
    }
    else
    {
        LOG_ERROR("RATE_SCALING_ITEMS not found\n");
        success = false;
    }

    complexIter = complexConstants.find("SPIRIT_FUSION_BOOST");
    if(success && complexIter != complexConstants.end())
    {
        std::unordered_map<std::string, std::string> map;
        success = LoadKeyValueStrings(complexIter->second, map);
        for(auto pair : map)
        {
            uint32_t key;
            if(!LoadInteger(pair.first, key))
            {
                LOG_ERROR("Failed to load SPIRIT_FUSION_BOOST key\n");
                success = false;
            }
            else if(sConstants.SPIRIT_FUSION_BOOST.find(key) !=
                sConstants.SPIRIT_FUSION_BOOST.end())
            {
                LOG_ERROR("Duplicate SPIRIT_FUSION_BOOST key encountered\n");
                success = false;
            }
            else
            {
                success &= ToIntegerArray(sConstants.SPIRIT_FUSION_BOOST[key],
                    libcomp::String(pair.second).Split(","));
            }

            if(!success)
            {
                break;
            }
        }
    }
    else
    {
        LOG_ERROR("SPIRIT_FUSION_BOOST not found\n");
        success = false;
    }

    complexIter = complexConstants.find("SYNTH_ADJUSTMENTS");
    if(success && complexIter != complexConstants.end())
    {
        std::unordered_map<std::string, std::string> map;
        success = LoadKeyValueStrings(complexIter->second, map);
        for(auto pair : map)
        {
            uint32_t key;
            if(!LoadInteger(pair.first, key))
            {
                LOG_ERROR("Failed to load SYNTH_ADJUSTMENTS key\n");
                success = false;
            }
            else if(sConstants.SYNTH_ADJUSTMENTS.find(key) !=
                sConstants.SYNTH_ADJUSTMENTS.end())
            {
                LOG_ERROR("Duplicate SYNTH_ADJUSTMENTS key encountered\n");
                success = false;
            }
            else
            {
                success &= ToIntegerArray(sConstants.SYNTH_ADJUSTMENTS[key],
                    libcomp::String(pair.second).Split(","));
            }

            if(!success)
            {
                break;
            }
        }
    }
    else
    {
        LOG_ERROR("SYNTH_ADJUSTMENTS not found\n");
        success = false;
    }
    
    complexIter = complexConstants.find("SYNTH_SKILLS");
    if(success && complexIter != complexConstants.end())
    {
        std::list<String> strList;
        if(!LoadStringList(complexIter->second, strList))
        {
            LOG_ERROR("Failed to load SYNTH_SKILLS\n");
            success = false;
        }
        else
        {
            if(strList.size() != 5)
            {
                LOG_ERROR("SYNTH_SKILLS must specify all five skill IDs\n");
                success = false;
            }
            else
            {
                size_t idx = 0;
                for(auto elem : strList)
                {
                    uint32_t skillID = 0;
                    if(LoadInteger(elem.C(), skillID))
                    {
                        sConstants.SYNTH_SKILLS[idx] = skillID;
                    }
                    else
                    {
                        LOG_ERROR("Failed to load a skill ID in"
                            " SLOT_MOD_ITEMS\n");
                        success = false;
                        break;
                    }

                    idx++;
                }
            }
        }
    }
    else
    {
        LOG_ERROR("SYNTH_SKILLS not found\n");
        success = false;
    }

    complexIter = complexConstants.find("TRIFUSION_SPECIAL_DARK");
    if(success && complexIter != complexConstants.end())
    {
        std::unordered_map<std::string, std::string> map;
        success = LoadKeyValueStrings(complexIter->second, map);
        for(auto pair : map)
        {
            uint8_t key;
            if(!LoadInteger(pair.first, key))
            {
                LOG_ERROR("Failed to load TRIFUSION_SPECIAL_DARK key\n");
                success = false;
            }
            else
            {
                uint32_t val = 0;
                if(LoadInteger(pair.second, val))
                {
                    sConstants.TRIFUSION_SPECIAL_DARK.push_back(
                        std::pair<uint8_t, uint32_t>(key, val));
                }
                else
                {
                    LOG_ERROR("Failed to load an element in"
                        " TRIFUSION_SPECIAL_DARK\n");
                    success = false;
                    break;
                }
            }

            if(!success)
            {
                break;
            }
        }

        sConstants.TRIFUSION_SPECIAL_DARK.sort([](
            const std::pair<uint8_t, uint32_t>& a,
            const std::pair<uint8_t, uint32_t>& b)
            {
                return a.first < b.first;
            });
    }
    else
    {
        LOG_ERROR("TRIFUSION_SPECIAL_DARK not found\n");
        success = false;
    }

    complexIter = complexConstants.find("TRIFUSION_SPECIAL_ELEMENTAL");
    if(success && complexIter != complexConstants.end())
    {
        std::list<String> strList;
        if(!LoadStringList(complexIter->second, strList))
        {
            LOG_ERROR("Failed to load TRIFUSION_SPECIAL_ELEMENTAL\n");
            success = false;
        }
        else
        {
            if(strList.size() != 6)
            {
                LOG_ERROR("TRIFUSION_SPECIAL_ELEMENTAL must specify all 6"
                    " two elemental combinations\n");
                success = false;
            }
            else
            {
                size_t idx = 0;
                for(auto elem : strList)
                {
                    success &= ToIntegerArray(
                        sConstants.TRIFUSION_SPECIAL_ELEMENTAL[idx++],
                        elem.Split(","));
                }
            }
        }
    }
    else
    {
        LOG_ERROR("TRIFUSION_SPECIAL_ELEMENTAL not found\n");
        success = false;
    }

    complexIter = complexConstants.find("VA_ADD_ITEM");
    if(success && complexIter != complexConstants.end())
    {
        std::list<String> strList;
        if(!LoadStringList(complexIter->second, strList))
        {
            LOG_ERROR("Failed to load VA_ADD_ITEM\n");
            success = false;
        }
        else
        {
            for(auto elemStr : strList)
            {
                uint32_t entry = 0;
                if(LoadInteger(elemStr.C(), entry))
                {
                    sConstants.VA_ADD_ITEM.insert(entry);
                }
                else
                {
                    LOG_ERROR("Failed to load an element in"
                        " VA_ADD_ITEM\n");
                    success = false;
                    break;
                }
            }
        }
    }
    else
    {
        LOG_ERROR("VA_ADD_ITEM not found\n");
        success = false;
    }

    return success;
}

const ServerConstants::Data& ServerConstants::GetConstants()
{
    return sConstants;
}

bool ServerConstants::LoadString(const std::string& value, String& prop)
{
    prop = value;
    return !value.empty();
}

bool ServerConstants::LoadStringList(const tinyxml2::XMLElement* elem,
    std::list<String>& prop)
{
    if(elem)
    {
        const tinyxml2::XMLElement* pElement = elem;
        if(std::string(pElement->Value()) != "element")
        {
            return false;
        }
        else
        {
            while(pElement)
            {
                libcomp::String elemStr(pElement && pElement->FirstChild()->ToText()
                    ? pElement->FirstChild()->ToText()->Value() : "");
                if(!elemStr.IsEmpty())
                {
                    prop.push_back(elemStr);
                }

                pElement = pElement->NextSiblingElement("element");
            }
        }
    }

    return true;
}

bool ServerConstants::LoadKeyValueStrings(const tinyxml2::XMLElement* elem,
    std::unordered_map<std::string, std::string>& map)
{
    if(elem)
    {
        const tinyxml2::XMLElement* pPair = elem;
        if(std::string(pPair->Value()) != "pair")
        {
            return false;
        }
        else
        {
            while(pPair)
            {
                const tinyxml2::XMLElement* pKey = pPair->FirstChildElement("key");
                const tinyxml2::XMLElement* pVal = pPair->FirstChildElement("value");

                std::string keyStr(pKey && pKey->FirstChild()->ToText()
                    ? pKey->FirstChild()->ToText()->Value() : "");
                std::string valStr(pVal && pVal->FirstChild()->ToText()
                    ? pVal->FirstChild()->ToText()->Value() : "");
                if(!keyStr.empty() || !valStr.empty())
                {
                    if(map.find(keyStr) != map.end())
                    {
                        return false;
                    }

                    map[keyStr] = valStr;
                }
                else
                {
                    return false;
                }

                pPair = pPair->NextSiblingElement("pair");
            }
        }
    }
    else
    {
        return false;
    }

    return true;
}
