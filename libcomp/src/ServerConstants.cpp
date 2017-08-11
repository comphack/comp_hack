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
    success &= LoadInteger(constants["ITEM_BALM_OF_LIFE"],
        sConstants.ITEM_BALM_OF_LIFE);

    // Load depo rental constants
    success &= LoadInteger(constants["RENTAL_DEMON_1"],
        sConstants.RENTAL_DEMON_1);
    success &= LoadInteger(constants["RENTAL_DEMON_3"],
        sConstants.RENTAL_DEMON_3);
    success &= LoadInteger(constants["RENTAL_DEMON_7"],
        sConstants.RENTAL_DEMON_7);
    success &= LoadInteger(constants["RENTAL_DEMON_30"],
        sConstants.RENTAL_DEMON_30);
    success &= LoadInteger(constants["RENTAL_DEMON_60"],
        sConstants.RENTAL_DEMON_60);
    success &= LoadInteger(constants["RENTAL_DEMON_90"],
        sConstants.RENTAL_DEMON_90);
    success &= LoadInteger(constants["RENTAL_ITEM_1"],
        sConstants.RENTAL_ITEM_1);
    success &= LoadInteger(constants["RENTAL_ITEM_3"],
        sConstants.RENTAL_ITEM_3);
    success &= LoadInteger(constants["RENTAL_ITEM_7"],
        sConstants.RENTAL_ITEM_7);
    success &= LoadInteger(constants["RENTAL_ITEM_30"],
        sConstants.RENTAL_ITEM_30);
    success &= LoadInteger(constants["RENTAL_ITEM_60"],
        sConstants.RENTAL_ITEM_60);
    success &= LoadInteger(constants["RENTAL_ITEM_90"],
        sConstants.RENTAL_ITEM_90);

    // Load skill constants
    success &= LoadInteger(constants["SKILL_CLAN_FORM"],
        sConstants.SKILL_CLAN_FORM);
    success &= LoadInteger(constants["SKILL_EQUIP_ITEM"],
        sConstants.SKILL_EQUIP_ITEM);
    success &= LoadInteger(constants["SKILL_EQUIP_ITEM"],
        sConstants.SKILL_EQUIP_ITEM);
    success &= LoadInteger(constants["SKILL_FAM_UP"],
        sConstants.SKILL_FAM_UP);
    success &= LoadInteger(constants["SKILL_ITEM_FAM_UP"],
        sConstants.SKILL_ITEM_FAM_UP);
    success &= LoadInteger(constants["SKILL_MOOCH"],
        sConstants.SKILL_MOOCH);
    success &= LoadInteger(constants["SKILL_STORE_DEMON"],
        sConstants.SKILL_STORE_DEMON);
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

    if(success && complexConstants.find("DEFAULT_SKILLS") != complexConstants.end())
    {
        const tinyxml2::XMLElement* pElement = complexConstants["DEFAULT_SKILLS"];
        if(std::string(pElement->Value()) != "element")
        {
            success = false;
        }
        else
        {
            while(pElement)
            {
                libcomp::String elemStr(pElement && pElement->FirstChild()->ToText()
                    ? pElement->FirstChild()->ToText()->Value() : "");
                if(!elemStr.IsEmpty())
                {
                    uint32_t entry = 0;
                    if(LoadInteger(elemStr.C(), entry))
                    {
                        sConstants.DEFAULT_SKILLS.insert(entry);
                    }
                    else
                    {
                        success = false;
                        break;
                    }
                }

                pElement = pElement->NextSiblingElement("element");
            }
        }
    }
    else
    {
        success = false;
    }

    if(success && complexConstants.find("CLAN_FORM_MAP") != complexConstants.end())
    {
        const tinyxml2::XMLElement* pPair = complexConstants["CLAN_FORM_MAP"];
        if(std::string(pPair->Value()) != "pair")
        {
            success = false;
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
                    uint32_t key = 0;
                    uint32_t val = 0;
                    if(LoadInteger(keyStr, key) && LoadInteger(valStr, val))
                    {
                        sConstants.CLAN_FORM_MAP[key] = val;
                    }
                    else
                    {
                        success = false;
                        break;
                    }
                }
                else
                {
                    success = false;
                    break;
                }

                pPair = pPair->NextSiblingElement("pair");
            }
        }
    }
    else
    {
        success = false;
    }

    if(success && complexConstants.find("CLAN_LEVEL_SKILLS") != complexConstants.end())
    {
        const tinyxml2::XMLElement* pElement = complexConstants["CLAN_LEVEL_SKILLS"];
        if(std::string(pElement->Value()) != "element")
        {
            success = false;
        }
        else
        {
            size_t idx = 0;
            while(pElement)
            {
                if(idx == 10)
                {
                    success = false;
                    break;
                }

                sConstants.CLAN_LEVEL_SKILLS[idx].clear();

                libcomp::String elemStr(pElement && pElement->FirstChild()->ToText()
                    ? pElement->FirstChild()->ToText()->Value() : "");
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
                            success = false;
                            break;
                        }
                    }
                }

                pElement = pElement->NextSiblingElement("element");
                idx++;
            }

            if(idx < 9)
            {
                success = false;
            }
        }
    }
    else
    {
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
