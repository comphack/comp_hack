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
    success &= LoadString(constants["EVENT_MENU_DEMON_DEPO"],
        sConstants.EVENT_MENU_DEMON_DEPO);
    success &= LoadString(constants["EVENT_MENU_HOMEPOINT"],
        sConstants.EVENT_MENU_HOMEPOINT);
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

    // Load skill constants
    success &= LoadInteger(constants["SKILL_CLAN_FORM"],
        sConstants.SKILL_CLAN_FORM);
    success &= LoadInteger(constants["SKILL_EQUIP_ITEM"],
        sConstants.SKILL_EQUIP_ITEM);
    success &= LoadInteger(constants["SKILL_EQUIP_MOD_EDIT"],
        sConstants.SKILL_EQUIP_MOD_EDIT);
    success &= LoadInteger(constants["SKILL_FAM_UP"],
        sConstants.SKILL_FAM_UP);
    success &= LoadInteger(constants["SKILL_ITEM_FAM_UP"],
        sConstants.SKILL_ITEM_FAM_UP);
    success &= LoadInteger(constants["SKILL_MOOCH"],
        sConstants.SKILL_MOOCH);
    success &= LoadInteger(constants["SKILL_REST"],
        sConstants.SKILL_REST);
    success &= LoadInteger(constants["SKILL_STORE_DEMON"],
        sConstants.SKILL_STORE_DEMON);
    success &= LoadInteger(constants["SKILL_SUICIDE"],
        sConstants.SKILL_SUICIDE);
    success &= LoadInteger(constants["SKILL_SUMMON_DEMON"],
        sConstants.SKILL_SUMMON_DEMON);
    success &= LoadInteger(constants["SKILL_TRAESTO"],
        sConstants.SKILL_TRAESTO);

    // Load status effect constants
    success &= LoadInteger(constants["STATUS_SUMMON_SYNC_1"],
        sConstants.STATUS_SUMMON_SYNC_1);
    success &= LoadInteger(constants["STATUS_SUMMON_SYNC_2"],
        sConstants.STATUS_SUMMON_SYNC_2);
    success &= LoadInteger(constants["STATUS_SUMMON_SYNC_3"],
        sConstants.STATUS_SUMMON_SYNC_3);

    auto complexIter = complexConstants.find("DEFAULT_SKILLS");
    if(success && complexIter != complexConstants.end())
    {
        std::list<String> strList;
        if(!LoadStringList(complexIter->second, strList))
        {
            LOG_ERROR("Failed to load DEFAULT_SKILLS\n");
            success = false;
        }
        else
        {
            for(auto elemStr : strList)
            {
                uint32_t entry = 0;
                if(LoadInteger(elemStr.C(), entry))
                {
                    sConstants.DEFAULT_SKILLS.insert(entry);
                }
                else
                {
                    LOG_ERROR("Failed to load an element in"
                        " DEFAULT_SKILLS\n");
                    success = false;
                    break;
                }
            }
        }
    }
    else
    {
        LOG_ERROR("DEFAULT_SKILLS not found\n");
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
                        for(auto elem : elemStr.Split(","))
                        {
                            uint32_t entry = 0;
                            if(LoadInteger(elem.C(), entry))
                            {
                                sConstants.CLAN_LEVEL_SKILLS[idx].insert(entry);
                            }
                            else
                            {
                                LOG_ERROR("Failed to load an element in"
                                    " CLAN_LEVEL_SKILLS\n");
                                success = false;
                                break;
                            }
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
                if(!pair.second.empty())
                {
                    auto params = libcomp::String(pair.second).Split(",");
                    if(params.size() == 3)
                    {
                        size_t idx = 0;
                        for(auto param : params)
                        {
                            int32_t p = 0;
                            if(LoadInteger(param.C(), p))
                            {
                                sConstants.EQUIP_MOD_EDIT_ITEMS[key][idx] = p;
                            }
                            else
                            {
                                LOG_ERROR("Failed to load an element in"
                                    " EQUIP_MOD_EDIT_ITEMS\n");
                                success = false;
                                break;
                            }

                            idx++;
                        }
                    }
                    else
                    {
                        LOG_ERROR("EQUIP_MOD_EDIT_ITEMS entry with param"
                            " count other than 3\n");
                        success = false;
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
        LOG_ERROR("EQUIP_MOD_EDIT_ITEMS not found\n");
        success = false;
    }
    
    complexIter = complexConstants.find("SLOT_MOD_ITEMS");
    if(success && complexIter != complexConstants.end())
    {
        std::list<String> strList;
        if(!LoadStringList(complexIter->second, strList))
        {
            LOG_ERROR("Failed to load SLOT_MOD_ITEMS\n");
            success = false;
        }
        else
        {
            if(strList.size() != 2)
            {
                LOG_ERROR("SLOT_MOD_ITEMS must specify items for both"
                    " of the 2 types\n");
                success = false;
            }
            else
            {
                size_t idx = 0;
                for(auto elemStr : strList)
                {
                    if(!elemStr.IsEmpty())
                    {
                        for(auto elem : elemStr.Split(","))
                        {
                            uint32_t entry = 0;
                            if(LoadInteger(elem.C(), entry))
                            {
                                sConstants.SLOT_MOD_ITEMS[idx].push_back(entry);
                            }
                            else
                            {
                                LOG_ERROR("Failed to load an element in"
                                    " SLOT_MOD_ITEMS\n");
                                success = false;
                                break;
                            }
                        }

                        if(sConstants.SLOT_MOD_ITEMS[idx].size() > 8)
                        {
                            LOG_ERROR("Each SLOT_MOD_ITEMS list must not exceed"
                                "the binary file array size of 8 possible items\n");
                            success = false;
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
        LOG_ERROR("SLOT_MOD_ITEMS not found\n");
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
                if(!pair.second.empty())
                {
                    auto params = libcomp::String(pair.second).Split(",");
                    if(params.size() == 3)
                    {
                        size_t idx = 0;
                        for(auto param : params)
                        {
                            int32_t p = 0;
                            if(LoadInteger(param.C(), p))
                            {
                                sConstants.SYNTH_ADJUSTMENTS[key][idx] = p;
                            }
                            else
                            {
                                LOG_ERROR("Failed to load an element in"
                                    " SYNTH_ADJUSTMENTS\n");
                                success = false;
                                break;
                            }

                            idx++;
                        }
                    }
                    else
                    {
                        LOG_ERROR("SYNTH_ADJUSTMENTS entry with param"
                            " count other than 3\n");
                        success = false;
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
            if(strList.size() != 3)
            {
                LOG_ERROR("SYNTH_SKILLS must specify all three skill IDs\n");
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
