/**
 * @file tools/bdpatch/src/main.cpp
 * @ingroup tools
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Tool to read and write BinaryData files.
 *
 * This tool will read and write BinaryData files.
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

// Standard C++11 Includes
#include <fstream>
#include <iostream>
#include <map>

// libcomp Includes
#include <Log.h>
#include <Object.h>

// object Includes
#include <MiAIData.h>
#include <MiBlendData.h>
#include <MiBlendExtData.h>
#include <MiCEventMessageData.h>
#include <MiCHouraiData.h>
#include <MiCIconData.h>
#include <MiCItemBaseData.h>
#include <MiCItemData.h>
#include <MiCMessageData.h>
#include <MiCModelBase.h>
#include <MiCModelData.h>
#include <MiCModifiedEffectData.h>
#include <MiCMultiTalkData.h>
#include <MiCPolygonMovieData.h>
#include <MiCQuestData.h>
#include <MiCSoundData.h>
#include <MiCTalkMessageData.h>
#include <MiCultureItemData.h>
#include <MiCZoneRelationData.h>
#include <MiDevilBookData.h>
#include <MiDevilBoostData.h>
#include <MiDevilBoostExtraData.h>
#include <MiDevilBoostItemData.h>
#include <MiDevilBoostLotData.h>
#include <MiDevilData.h>
#include <MiDevilEquipmentData.h>
#include <MiDevilEquipmentItemData.h>
#include <MiDevilFusionData.h>
#include <MiDevilLVUpRateData.h>
#include <MiDisassemblyData.h>
#include <MiDisassemblyTriggerData.h>
#include <MiDynamicMapData.h>
#include <MiEnchantData.h>
#include <MiEquipmentSetData.h>
#include <MiEventDirectionData.h>
#include <MiExchangeData.h>
#include <MiExpertClassData.h>
#include <MiGuardianAssistData.h>
#include <MiGuardianLevelData.h>
#include <MiGuardianSpecialData.h>
#include <MiGuardianUnlockData.h>
#include <MiHNPCBasicData.h>
#include <MiHNPCData.h>
#include <MiItemData.h>
#include <MiMissionData.h>
#include <MiMitamaReunionBonusData.h>
#include <MiMitamaReunionSetBonusData.h>
#include <MiMitamaUnionBonusData.h>
#include <MiModificationData.h>
#include <MiModificationExtEffectData.h>
#include <MiModificationExtRecipeData.h>
#include <MiModificationTriggerData.h>
#include <MiModifiedEffectData.h>
#include <MiMultiTalkCmdTbl.h>
#include <MiNextEpisodeInfo.h>
#include <MiNPCBarterData.h>
#include <MiNPCBasicData.h>
#include <MiONPCData.h>
#include <MiPMAttachCharacterTbl.h>
#include <MiPMBaseInfo.h>
#include <MiPMBGMKeyTbl.h>
#include <MiPMCameraKeyTbl.h>
#include <MiPMEffectKeyTbl.h>
#include <MiPMFadeKeyTbl.h>
#include <MiPMFogKeyTbl.h>
#include <MiPMGouraudKeyTbl.h>
#include <MiPMMotionKeyTbl.h>
#include <MiPMMsgKeyTbl.h>
#include <MiPMScalingHelperTbl.h>
#include <MiPMSEKeyTbl.h>
#include <MiQuestBonusCodeData.h>
#include <MiQuestData.h>
#include <MiShopProductData.h>
#include <MiSItemData.h>
#include <MiSkillData.h>
#include <MiSkillItemStatusCommonData.h>
#include <MiSpotData.h>
#include <MiStatusData.h>
#include <MiStatusData.h>
#include <MiSynthesisData.h>
#include <MiTimeLimitData.h>
#include <MiTitleData.h>
#include <MiTriUnionSpecialData.h>
#include <MiUraFieldTowerData.h>
#include <MiWarpPointData.h>
#include <MiZoneBasicData.h>
#include <MiZoneData.h>

// tinyxml2 Includes
#include <PushIgnore.h>
#include <tinyxml2.h>
#include <PopIgnore.h>

class BinaryDataSet
{
public:
    BinaryDataSet(std::function<std::shared_ptr<libcomp::Object>()> allocator,
        std::function<uint32_t(const std::shared_ptr<
            libcomp::Object>&)> mapper);

    bool Load(std::istream& file);
    bool Save(std::ostream& file) const;

    bool LoadXml(tinyxml2::XMLDocument& doc);

    std::string GetXml() const;
    std::string GetTabular() const;

    std::list<std::shared_ptr<libcomp::Object>> GetObjects() const;
    std::shared_ptr<libcomp::Object> GetObjectByID(uint32_t id) const;

private:
    std::list<std::string> ReadNodes(tinyxml2::XMLElement *node,
        int16_t dataMode) const;

    std::function<std::shared_ptr<libcomp::Object>()> mObjectAllocator;
    std::function<uint32_t(const std::shared_ptr<
        libcomp::Object>&)> mObjectMapper;

    std::list<std::shared_ptr<libcomp::Object>> mObjects;
    std::map<uint32_t, std::shared_ptr<libcomp::Object>> mObjectMap;
};

BinaryDataSet::BinaryDataSet(std::function<std::shared_ptr<
    libcomp::Object>()> allocator, std::function<uint32_t(
    const std::shared_ptr<libcomp::Object>&)> mapper) :
    mObjectAllocator(allocator), mObjectMapper(mapper)
{
}

std::list<std::string> BinaryDataSet::ReadNodes(tinyxml2::XMLElement *node,
    int16_t dataMode) const
{
    std::list<std::string> data;
    std::list<tinyxml2::XMLElement*> nodes;
    while(node != nullptr)
    {
        if(node->FirstChildElement() != nullptr)
        {
            nodes.push_back(node);
            node = node->FirstChildElement();

            //Doesn't handle map
            if(std::string(node->Name()) == "element")
            {
                if(dataMode > 1)
                {
                    std::list<std::string> nData;
                    while(node != nullptr)
                    {
                        std::list<std::string> sData;
                        if(node->FirstChildElement() != nullptr)
                        {
                            sData = ReadNodes(node->FirstChildElement(), 3);
                        }
                        else
                        {
                            const char* txt = node->GetText();
                            auto str = txt != 0 ? std::string(txt) : std::string();
                            sData.push_back(str);
                        }

                        std::stringstream ss;
                        ss << "{ ";
                        bool first = true;
                        for(auto d : sData)
                        {
                            if(!first) ss << ", ";
                            ss << d;
                            first = false;
                        }
                        ss << " }";
                        nData.push_back(ss.str());

                        node = node->NextSiblingElement();
                    }

                    std::stringstream ss;
                    bool first = true;
                    for(auto d : nData)
                    {
                        if(!first) ss << ", ";
                        ss << d;
                        first = false;
                    }
                    data.push_back(ss.str());
                }
                else
                {
                    //Pull the name and skip
                    data.push_back(std::string(nodes.back()->Attribute("name")));
                    node = nullptr;
                }
            }
        }
        else
        {
            const char* txt = node->GetText();
            auto name = std::string(node->Attribute("name"));
            auto val = txt != 0 ? std::string(txt) : std::string();
            val.erase(std::remove(val.begin(), val.end(), '\n'), val.end());
            switch(dataMode)
            {
                case 1:
                    data.push_back(name);
                    break;
                case 2:
                    data.push_back(val);
                    break;
                default:
                    data.push_back(name + ": " + val);
                    break;
            }

            node = node->NextSiblingElement();
        }

        while(node == nullptr && nodes.size() > 0)
        {
            node = nodes.back();
            nodes.pop_back();
            node = node->NextSiblingElement();
        }
    }

    return data;
}

std::string BinaryDataSet::GetTabular() const
{
    tinyxml2::XMLDocument doc;

    tinyxml2::XMLElement *pRoot = doc.NewElement("objects");
    doc.InsertEndChild(pRoot);

    for(auto obj : mObjects)
    {
        if(!obj->Save(doc, *pRoot))
        {
            return {};
        }
    }

    std::stringstream ss;

    tinyxml2::XMLNode *pNode = doc.FirstChild()->FirstChild();

    if(pNode != nullptr)
    {
        for(auto d : ReadNodes(pNode->FirstChildElement(), 1))
        {
            ss << d << "\t";
        }

        ss << std::endl;
    }

    while(pNode != nullptr)
    {
        std::list<tinyxml2::XMLElement*> nodes;

        tinyxml2::XMLElement *cNode = pNode->FirstChildElement();
        auto data = ReadNodes(cNode, 2);
        for(auto d : data)
        {
            ss << d << "\t";
        }

        ss << std::endl;
        pNode = pNode->NextSiblingElement();
    }

    return ss.str();
}

bool BinaryDataSet::Load(std::istream& file)
{
    mObjects = libcomp::Object::LoadBinaryData(file,
        mObjectAllocator);

    for(auto obj : mObjects)
    {
        mObjectMap[mObjectMapper(obj)] = obj;
    }

    return !mObjects.empty();
}

bool BinaryDataSet::Save(std::ostream& file) const
{
    return libcomp::Object::SaveBinaryData(file, mObjects);
}

bool BinaryDataSet::LoadXml(tinyxml2::XMLDocument& doc)
{
    std::list<std::shared_ptr<libcomp::Object>> objs;

    auto pRoot = doc.RootElement();

    if(nullptr == pRoot)
    {
        return false;
    }

    auto objElement = pRoot->FirstChildElement("object");

    while(nullptr != objElement)
    {
        auto obj = mObjectAllocator();

        if(!obj->Load(doc, *objElement))
        {
            return false;
        }

        objs.push_back(obj);

        objElement = objElement->NextSiblingElement("object");
    }

    mObjects = objs;
    mObjectMap.clear();

    for(auto obj : mObjects)
    {
        mObjectMap[mObjectMapper(obj)] = obj;
    }

    return !mObjects.empty();
}

std::string BinaryDataSet::GetXml() const
{
    tinyxml2::XMLDocument doc;

    tinyxml2::XMLElement *pRoot = doc.NewElement("objects");
    doc.InsertEndChild(pRoot);

    for(auto obj : mObjects)
    {
        if(!obj->Save(doc, *pRoot))
        {
            return {};
        }
    }

    tinyxml2::XMLPrinter printer;
    doc.Print(&printer);

    return printer.CStr();
}

std::list<std::shared_ptr<libcomp::Object>> BinaryDataSet::GetObjects() const
{
    return mObjects;
}

std::shared_ptr<libcomp::Object> BinaryDataSet::GetObjectByID(
    uint32_t id) const
{
    auto it = mObjectMap.find(id);

    if(mObjectMap.end() != it)
    {
        return it->second;
    }

    return {};
}

int Usage(const char *szAppName, const std::map<std::string,
    std::pair<std::string, std::function<BinaryDataSet*(void)>>>& binaryTypes)
{
    std::cerr << "USAGE: " << szAppName << " load TYPE IN OUT" << std::endl;
    std::cerr << "USAGE: " << szAppName << " save TYPE IN OUT" << std::endl;
    std::cerr << "USAGE: " << szAppName << " flatten TYPE IN OUT" << std::endl;
    std::cerr << std::endl;
    std::cerr << "TYPE indicates the format of the BinaryData and can "
        << "be one of:" << std::endl;

    for(auto& typ : binaryTypes)
    {
        std::cerr << typ.second.first << std::endl;
    }

    std::cerr << std::endl;
    std::cerr << "Mode 'load' will take the input BinaryData file and "
        << "write the output XML file." << std::endl;
    std::cerr << std::endl;
    std::cerr << "Mode 'save' will take the input XML file and "
        << "write the output BinaryData file." << std::endl;

    return EXIT_FAILURE;
}

#define ADD_TYPE(desc, key, objname) \
    binaryTypes[key] = std::make_pair(desc, []() \
    { \
        return new BinaryDataSet( \
            []() \
            { \
                return std::make_shared<objects::objname>(); \
            }, \
\
            [](const std::shared_ptr<libcomp::Object>& obj) -> uint32_t \
            { \
                return static_cast<uint32_t>(std::dynamic_pointer_cast< \
                    objects::objname>(obj)->GetID()); \
            } \
        ); \
    });

#define ADD_TYPE_EX(desc, key, objname, getid) \
    binaryTypes[key] = std::make_pair(desc, []() \
    { \
\
        return new BinaryDataSet( \
            []() \
            { \
                return std::make_shared<objects::objname>(); \
            }, \
\
            [](const std::shared_ptr<libcomp::Object>& obj) -> uint32_t \
            { \
                return static_cast<uint32_t>(std::dynamic_pointer_cast< \
                    objects::objname>(obj)->getid); \
            } \
        ); \
    });

#define ADD_TYPE_SEQ(desc, key, objname) \
    binaryTypes[key] = std::make_pair(desc, []() \
    { \
        static uint32_t nextID = 0; \
\
        return new BinaryDataSet( \
            []() \
            { \
                return std::make_shared<objects::objname>(); \
            }, \
\
            [](const std::shared_ptr<libcomp::Object>& obj) \
            { \
                (void)obj; \
\
                return nextID++; \
            } \
        ); \
    });

int main(int argc, char *argv[])
{
    std::map<std::string, std::pair<std::string,
        std::function<BinaryDataSet*(void)>>> binaryTypes;

    ADD_TYPE    ("  ai              Format for AIData.sbin", "ai", MiAIData);
    ADD_TYPE    ("  blend           Format for BlendData.sbin", "blend", MiBlendData);
    ADD_TYPE    ("  blendext        Format for BlendExtData.sbin", "blendext", MiBlendExtData);
    ADD_TYPE    ("  ceventmessage   Format for CEventMessageData.sbin", "ceventmessage", MiCEventMessageData);
    ADD_TYPE    ("  chourai         Format for CHouraiData.sbin", "chourai", MiCHouraiData);
    ADD_TYPE    ("  cicon           Format for CIconData.bin", "cicon", MiCIconData);
    ADD_TYPE    ("  cmessage        Format for CMessageData.sbin", "cmessage", MiCMessageData);
    ADD_TYPE    ("  cmodifiedeffect Format for CMessageData.sbin", "cmodifiedeffect", MiCModifiedEffectData);
    ADD_TYPE    ("  cmultitalk      Format for CMultiTalkData.sbin", "cmultitalk", MiCMultiTalkData);
    ADD_TYPE    ("  cquest          Format for CQuestData.sbin", "cquest", MiCQuestData);
    ADD_TYPE    ("  csound          Format for CSoundData.bin", "csound", MiCSoundData);
    ADD_TYPE    ("  ctalkmessage    Format for CTalkMessageData.sbin", "ctalkmessage", MiCTalkMessageData);
    ADD_TYPE    ("  cultureitem     Format for CultureItemData.bin", "cultureitem", MiCultureItemData);
    ADD_TYPE    ("  czonerelation   Format for CZoneRelationData.sbin", "czonerelation", MiCZoneRelationData);
    ADD_TYPE    ("  devilbook       Format for DevilBookData.sbin", "devilbook", MiDevilBookData);
    ADD_TYPE    ("  devilboost      Format for DevilBoostData.sbin", "devilboost", MiDevilBoostData);
    ADD_TYPE    ("  devillvluprate  Format for DevilLVUpRateData.sbin", "devillvluprate", MiDevilLVUpRateData);
    ADD_TYPE    ("  disassembly     Format for DisassemblyData.sbin", "disassembly", MiDisassemblyData);
    ADD_TYPE    ("  disassemblytrig Format for DisassemblyTriggerData.sbin", "disassemblytrig", MiDisassemblyTriggerData);
    ADD_TYPE    ("  dynamicmap      Format for DynamicMapData.bin", "dynamicmap", MiDynamicMapData);
    ADD_TYPE    ("  enchant         Format for EnchantData.sbin", "enchant", MiEnchantData);
    ADD_TYPE    ("  equipset        Format for EquipmentSetData.sbin", "equipset", MiEquipmentSetData);
    ADD_TYPE    ("  eventdirection  Format for EventDirectionData.bin", "eventdirection", MiEventDirectionData);
    ADD_TYPE    ("  exchange        Format for ExchangeData.sbin", "exchange", MiExchangeData);
    ADD_TYPE    ("  expertclass     Format for ExpertClassData.sbin", "expertclass", MiExpertClassData);
    ADD_TYPE    ("  guardianassist  Format for GuardianAssistData.sbin", "guardianassist", MiGuardianAssistData);
    ADD_TYPE    ("  guardianlevel   Format for GuardianLevelData.sbin", "guardianlevel", MiGuardianLevelData);
    ADD_TYPE    ("  guardianspecial Format for GuardianSpecialData.sbin", "guardianspecial", MiGuardianSpecialData);
    ADD_TYPE    ("  guardianunlock  Format for GuardianUnlockData.sbin", "guardianunlock", MiGuardianUnlockData);
    ADD_TYPE    ("  mission         Format for MissionData.sbin", "mission", MiMissionData);
    ADD_TYPE    ("  mitamabonus     Format for MitamaReunionBonusData.sbin", "mitamabonus", MiMitamaReunionBonusData);
    ADD_TYPE    ("  mitamasetbonus  Format for MitamaReunionSetBonusData.sbin", "mitamasetbonus", MiMitamaReunionSetBonusData);
    ADD_TYPE    ("  mitamaunion     Format for MitamaUnionBonusData.sbin", "mitamaunion", MiMitamaUnionBonusData);
    ADD_TYPE    ("  mod             Format for ModificationData.sbin", "mod", MiModificationData);
    ADD_TYPE    ("  modeffect       Format for ModifiedEffectData.sbin", "modeffect", MiModifiedEffectData);
    ADD_TYPE    ("  modextrecipe    Format for ModificationExtRecipeData.sbin", "modextrecipe", MiModificationExtRecipeData);
    ADD_TYPE    ("  modtrigger      Format for ModificationTriggerData.sbin", "modtrigger", MiModificationTriggerData);
    ADD_TYPE    ("  npcbarter       Format for NPCBarterData.sbin", "npcbarter", MiNPCBarterData);
    ADD_TYPE    ("  onpc            Format for oNPCData.sbin", "onpc", MiONPCData);
    ADD_TYPE    ("  quest           Format for QuestData.sbin", "quest", MiQuestData);
    ADD_TYPE    ("  questbonuscode  Format for QuestBonusCodeData.sbin", "questbonuscode", MiQuestBonusCodeData);
    ADD_TYPE    ("  shopproduct     Format for ShopProductData.sbin", "shopproduct", MiShopProductData);
    ADD_TYPE    ("  sitem           Format for SItemData.sbin", "sitem", MiSItemData);
    ADD_TYPE    ("  spot            Format for SpotData.bin", "spot", MiSpotData);
    ADD_TYPE    ("  synthesis       Format for SynthesisData.sbin", "synthesis", MiSynthesisData);
    ADD_TYPE    ("  timelimit       Format for TimeLimitData.sbin", "timelimit", MiTimeLimitData);
    ADD_TYPE    ("  title           Format for CodeNameData.sbin", "title", MiTitleData);
    ADD_TYPE    ("  triunionspecial Format for TriUnionSpecialData.sbin", "triunionspecial", MiTriUnionSpecialData);
    ADD_TYPE    ("  urafieldtower   Format for UraFieldTowerData.sbin", "urafieldtower", MiUraFieldTowerData);
    ADD_TYPE    ("  warppoint       Format for WarpPointData.sbin", "warppoint", MiWarpPointData);
    ADD_TYPE_EX ("  citem           Format for CItemData.sbin", "citem", MiCItemData, GetBaseData()->GetID());
    ADD_TYPE_EX ("  cmodel          Format for CModelData.sbin", "cmodel", MiCModelData, GetBase()->GetID());
    ADD_TYPE_EX ("  devil           Format for DevilData.sbin", "devil", MiDevilData, GetBasic()->GetID());
    ADD_TYPE_EX ("  devilboostextra Format for DevilBoostExtraData.sbin", "devilboostextra", MiDevilBoostExtraData, GetItemID());
    ADD_TYPE_EX ("  devilboostitem  Format for DevilBoostItemData.sbin", "devilboostitem", MiDevilBoostItemData, GetItemID());
    ADD_TYPE_EX ("  devilboostlot   Format for DevilBoostLotData.sbin", "devilboostlot", MiDevilBoostLotData, GetLot());
    ADD_TYPE_EX ("  devilequip      Format for DevilEquipmentData.sbin", "devilequip", MiDevilEquipmentData, GetSkillID());
    ADD_TYPE_EX ("  devilequipitem  Format for DevilEquipmentItemData.sbin", "devilequipitem", MiDevilEquipmentItemData, GetItemID());
    ADD_TYPE_EX ("  devilfusion     Format for DevilFusionData.sbin", "devilfusion", MiDevilFusionData, GetSkillID());
    ADD_TYPE_EX ("  hnpc            Format for hNPCData.sbin", "hnpc", MiHNPCData, GetBasic()->GetID());
    ADD_TYPE_EX ("  item            Format for ItemData.sbin", "item", MiItemData, GetCommon()->GetID());
    ADD_TYPE_EX ("  modexteffect    Format for ModificationExtEffectData.sbin", "modexteffect", MiModificationExtEffectData, GetSubID());
    ADD_TYPE_EX ("  skill           Format for SkillData.sbin", "skill", MiSkillData, GetCommon()->GetID());
    ADD_TYPE_EX ("  status          Format for StatusData.sbin", "status", MiStatusData, GetCommon()->GetID());
    ADD_TYPE_EX ("  zone            Format for ZoneData.sbin", "zone", MiZoneData, GetBasic()->GetID());
    ADD_TYPE_SEQ("  cpolygonmovie   Format for CPolygonMoveData.sbin", "cpolygonmovie", MiCPolygonMovieData);

    if(5 != argc)
    {
        return Usage(argv[0], binaryTypes);
    }

    libcomp::Log::GetSingletonPtr()->AddStandardOutputHook();

    libcomp::String mode = argv[1];
    libcomp::String bdType = argv[2];
    const char *szInPath = argv[3];
    const char *szOutPath = argv[4];

    if("load" != mode && "save" != mode && "flatten" != mode)
    {
        return Usage(argv[0], binaryTypes);
    }

    BinaryDataSet *pSet = nullptr;

    auto match = binaryTypes.find(bdType.ToUtf8());

    if(binaryTypes.end() != match)
    {
        pSet = (match->second.second)();
    }

    if(!pSet)
    {
        return Usage(argv[0], binaryTypes);
    }

    if("load" == mode)
    {
        std::ifstream file;
        file.open(szInPath, std::ifstream::binary);

        if(!pSet->Load(file))
        {
            std::cerr << "Failed to load file: " << szInPath << std::endl;

            return EXIT_FAILURE;
        }

        std::ofstream out;
        out.open(szOutPath);

        out << pSet->GetXml().c_str();

        if(!out.good())
        {
            std::cerr << "Failed to save file: " << szOutPath << std::endl;

            return EXIT_FAILURE;
        }
    }
    else if("flatten" == mode)
    {
        std::ifstream file;
        file.open(szInPath, std::ifstream::binary);

        if(!pSet->Load(file))
        {
            std::cerr << "Failed to load file: " << szInPath << std::endl;

            return EXIT_FAILURE;
        }

        std::ofstream out;
        out.open(szOutPath);

        out << pSet->GetTabular().c_str();

        if(!out.good())
        {
            std::cerr << "Failed to save file: " << szOutPath << std::endl;

            return EXIT_FAILURE;
        }
    }
    else if("save" == mode)
    {
        tinyxml2::XMLDocument doc;

        if(tinyxml2::XML_NO_ERROR != doc.LoadFile(szInPath))
        {
            std::cerr << "Failed to parse file: " << szInPath << std::endl;

            return EXIT_FAILURE;
        }

        if(!pSet->LoadXml(doc))
        {
            std::cerr << "Failed to load file: " << szInPath << std::endl;

            return EXIT_FAILURE;
        }

        std::ofstream out;
        out.open(szOutPath, std::ofstream::binary);

        if(!pSet->Save(out))
        {
            std::cerr << "Failed to save file: " << szOutPath << std::endl;

            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
