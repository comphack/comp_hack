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
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
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
#include <BinaryDataSet.h>
#include <Constants.h>
#include <Log.h>
#include <Object.h>

// object Includes
#include <MiAIData.h>
#include <MiBazaarClerkNPCData.h>
#include <MiBlendData.h>
#include <MiBlendExtData.h>
#include <MiCAppearanceEquipData.h>
#include <MiCChanceItemData.h>
#include <MiCCultureData.h>
#include <MiCDevilBookBonusData.h>
#include <MiCDevilBookBonusMitamaData.h>
#include <MiCDevilBoostIconData.h>
#include <MiCDevilDungeonData.h>
#include <MiCDevilEquipmentExclusiveData.h>
#include <MiCEquipModelData.h>
#include <MiCEventData.h>
#include <MiCEventMessageData.h>
#include <MiCGuardianAssistData.h>
#include <MiCHelpData.h>
#include <MiCHouraiData.h>
#include <MiCHouraiMessageData.h>
#include <MiCIconData.h>
#include <MiCItemBaseData.h>
#include <MiCItemData.h>
#include <MiCKeyItemData.h>
#include <MiCLoadingCommercialData.h>
#include <MiCMapData.h>
#include <MiCMessageData.h>
#include <MiCModelBase.h>
#include <MiCModelData.h>
#include <MiCModifiedEffectData.h>
#include <MiCMultiTalkData.h>
#include <MiCMultiTalkDirectionData.h>
#include <MiCMultiTalkPopData.h>
#include <MiCPolygonMovieData.h>
#include <MiCQuestData.h>
#include <MiCSkillBase.h>
#include <MiCSkillData.h>
#include <MiCSoundData.h>
#include <MiCSpecialSkillEffectData.h>
#include <MiCStatusData.h>
#include <MiCTalkMessageData.h>
#include <MiCTimeAttackData.h>
#include <MiCTitleData.h>
#include <MiCTransformedModelData.h>
#include <MiCValuablesData.h>
#include <MiCultureItemData.h>
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
#include <MiExpertData.h>
#include <MiGuardianAssistData.h>
#include <MiGuardianLevelData.h>
#include <MiGuardianSpecialData.h>
#include <MiGuardianUnlockData.h>
#include <MiGvGTrophyData.h>
#include <MiHNPCBasicData.h>
#include <MiHNPCData.h>
#include <MiItemData.h>
#include <MiKeyItemData.h>
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
#include <MiNPCBarterConditionData.h>
#include <MiNPCBarterData.h>
#include <MiNPCBarterGroupData.h>
#include <MiNPCBarterTextData.h>
#include <MiNPCBasicData.h>
#include <MiNPCInvisibleData.h>
#include <MiNextEpisodeInfo.h>
#include <MiONPCData.h>
#include <MiPMAttachCharacterTbl.h>
#include <MiPMBGMKeyTbl.h>
#include <MiPMBaseInfo.h>
#include <MiPMCameraKeyTbl.h>
#include <MiPMEffectKeyTbl.h>
#include <MiPMFadeKeyTbl.h>
#include <MiPMFogKeyTbl.h>
#include <MiPMGouraudKeyTbl.h>
#include <MiPMMotionKeyTbl.h>
#include <MiPMMsgKeyTbl.h>
#include <MiPMSEKeyTbl.h>
#include <MiPMScalingHelperTbl.h>
#include <MiQuestBonusCodeData.h>
#include <MiQuestBonusData.h>
#include <MiQuestData.h>
#include <MiReportTypeData.h>
#include <MiSItemData.h>
#include <MiShopProductData.h>
#include <MiSkillData.h>
#include <MiSkillItemStatusCommonData.h>
#include <MiSpotData.h>
#include <MiStatusData.h>
#include <MiSynthesisData.h>
#include <MiTankData.h>
#include <MiTimeLimitData.h>
#include <MiTitleData.h>
#include <MiTriUnionSpecialData.h>
#include <MiUIInfoData.h>
#include <MiUraFieldTowerData.h>
#include <MiWarpPointData.h>
#include <MiZoneBasicData.h>
#include <MiZoneData.h>
#include <QmpFile.h>

#ifdef DREAM_OBJGEN_INCLUDE_A
#include DREAM_OBJGEN_INCLUDE_A
#endif  // DREAM_OBJGEN_INCLUDE_A

// Ignore warnings
#include <PushIgnore.h>

// tinyxml2 Includes
#include <tinyxml2.h>

// Stop ignoring warnings
#include <PopIgnore.h>

class ManualBinaryDataSet : public libcomp::BinaryDataSet {
 public:
  ManualBinaryDataSet(
      std::function<std::shared_ptr<libcomp::Object>()> alloc,
      std::function<uint32_t(const std::shared_ptr<libcomp::Object>&)> map)
      : libcomp::BinaryDataSet(alloc, map) {}
  virtual ~ManualBinaryDataSet() {}

  void AddRecord(std::shared_ptr<libcomp::Object> obj) {
    mObjects.push_back(obj);
  }
};

int Usage(
    const char* szAppName,
    const std::map<
        std::string,
        std::pair<std::string, std::function<libcomp::BinaryDataSet*(void)>>>&
        binaryTypes) {
  std::cerr << "USAGE: " << szAppName << " load TYPE IN OUT" << std::endl;
  std::cerr << "USAGE: " << szAppName << " save TYPE IN OUT" << std::endl;
  std::cerr << "USAGE: " << szAppName << " flatten TYPE IN OUT" << std::endl;
  std::cerr << std::endl;
  std::cerr << "TYPE indicates the format of the BinaryData and can "
            << "be one of:" << std::endl;

  for (auto& typ : binaryTypes) {
    std::cerr << typ.second.first << std::endl;
  }

  std::cerr << std::endl;
  std::cerr << "Mode 'load' will take the input BinaryData file and "
            << "write the output XML file." << std::endl;
  std::cerr << std::endl;
  std::cerr << "Mode 'save' will take the input XML file and "
            << "write the output BinaryData file." << std::endl;
  std::cerr << std::endl;
  std::cerr << "Mode 'flatten' will take the input BinaryData file and "
            << "write the output text file." << std::endl;

  return EXIT_FAILURE;
}

#define ADD_TYPE(desc, key, objname)                                      \
  binaryTypes[key] = std::make_pair(desc, []() {                          \
    return new libcomp::BinaryDataSet(                                    \
        []() { return std::make_shared<objects::objname>(); },            \
                                                                          \
        [](const std::shared_ptr<libcomp::Object>& obj) -> uint32_t {     \
          return static_cast<uint32_t>(                                   \
              std::dynamic_pointer_cast<objects::objname>(obj)->GetID()); \
        });                                                               \
  });

#define ADD_TYPE_EX(desc, key, objname, getid)                          \
  binaryTypes[key] = std::make_pair(desc, []() {                        \
    return new libcomp::BinaryDataSet(                                  \
        []() { return std::make_shared<objects::objname>(); },          \
                                                                        \
        [](const std::shared_ptr<libcomp::Object>& obj) -> uint32_t {   \
          return static_cast<uint32_t>(                                 \
              std::dynamic_pointer_cast<objects::objname>(obj)->getid); \
        });                                                             \
  });

#define ADD_TYPE_SEQ(desc, key, objname)                       \
  binaryTypes[key] = std::make_pair(desc, []() {               \
    static uint32_t nextID = 0;                                \
                                                               \
    return new libcomp::BinaryDataSet(                         \
        []() { return std::make_shared<objects::objname>(); }, \
                                                               \
        [](const std::shared_ptr<libcomp::Object>& obj) {      \
          (void)obj;                                           \
                                                               \
          return nextID++;                                     \
        });                                                    \
  });

#define ADD_TYPE_MAN(desc, key, objname)                       \
  binaryTypes[key] = std::make_pair(desc, []() {               \
    static uint32_t nextID = 0;                                \
                                                               \
    return new ManualBinaryDataSet(                            \
        []() { return std::make_shared<objects::objname>(); }, \
                                                               \
        [](const std::shared_ptr<libcomp::Object>& obj) {      \
          (void)obj;                                           \
                                                               \
          return nextID++;                                     \
        });                                                    \
  });

int main(int argc, char* argv[]) {
  std::map<std::string,
           std::pair<std::string, std::function<libcomp::BinaryDataSet*(void)>>>
      binaryTypes;

  // clang-format off
  ADD_TYPE    ("  ai                    Format for AIData.sbin", "ai", MiAIData);
  ADD_TYPE    ("  bazaarclerknpc        Format for BazaarClerkNPCData.sbin", "bazaarclerknpc", MiBazaarClerkNPCData);
  ADD_TYPE    ("  blend                 Format for BlendData.sbin", "blend", MiBlendData);
  ADD_TYPE    ("  blendext              Format for BlendExtData.sbin", "blendext", MiBlendExtData);
  ADD_TYPE    ("  cappearanceequip      Format for CAppearanceEquipData.bin", "cappearanceequip", MiCAppearanceEquipData);
  ADD_TYPE    ("  cchanceitem           Format for CChanceItemData.sbin", "cchanceitem", MiCChanceItemData);
  ADD_TYPE    ("  cdevilbookbonus       Format for CDevilBookBonusData.sbin", "cdevilbookbonus", MiCDevilBookBonusData);
  ADD_TYPE    ("  cdevilbookbonusmitama Format for CDevilBookBonusMitamaData.sbin", "cdevilbookbonusmitama", MiCDevilBookBonusMitamaData);
  ADD_TYPE    ("  cdevilboosticon       Format for CDevilBoostIconData.sbin", "cdevilboosticon", MiCDevilBoostIconData);
  ADD_TYPE    ("  cdevildungeon         Format for CDevilDungeonData.sbin", "cdevildungeon", MiCDevilDungeonData);
  ADD_TYPE    ("  cdevilequipexclusive  Format for CDevilEquipmentExclusiveData.sbin", "cdevilequipexclusive", MiCDevilEquipmentExclusiveData);
  ADD_TYPE    ("  cequipmodel           Format for CEquipModelData.sbin", "cequipmodel", MiCEquipModelData);
  ADD_TYPE    ("  cevent                Format for CEventData.bin", "cevent", MiCEventData);
  ADD_TYPE    ("  ceventmessage         Format for CEventMessageData.sbin", "ceventmessage", MiCEventMessageData);
  ADD_TYPE    ("  cguardianassist       Format for CGuardianAssistData.sbin", "cguardianassist", MiCGuardianAssistData);
  ADD_TYPE    ("  chelp                 Format for CHelpData.sbin", "chelp", MiCHelpData);
  ADD_TYPE    ("  chourai               Format for CHouraiData.sbin", "chourai", MiCHouraiData);
  ADD_TYPE    ("  chouraimessage        Format for CHouraiMessageData.sbin", "chouraimessage", MiCHouraiMessageData);
  ADD_TYPE    ("  cicon                 Format for CIconData.bin", "cicon", MiCIconData);
  ADD_TYPE    ("  cloadingcommercial    Format for CLoadingCommercialData.sbin", "cloadingcommercial", MiCLoadingCommercialData);
  ADD_TYPE    ("  cmap                  Format for CMapData.bin", "cmap", MiCMapData);
  ADD_TYPE    ("  cmessage              Format for CMessageData.sbin", "cmessage", MiCMessageData);
  ADD_TYPE    ("  cmodifiedeffect       Format for CModifiedEffectData.sbin", "cmodifiedeffect", MiCModifiedEffectData);
  ADD_TYPE    ("  cmultitalk            Format for CMultiTalkData.bin", "cmultitalk", MiCMultiTalkData);
  ADD_TYPE    ("  cmultitalkdirection   Format for CMultiTalkDirectionData.bin", "cmultitalkdirection", MiCMultiTalkDirectionData);
  ADD_TYPE    ("  cmultitalkpop         Format for CMultiTalkPopData.bin", "cmultitalkpop", MiCMultiTalkPopData);
  ADD_TYPE    ("  cquest                Format for CQuestData.sbin", "cquest", MiCQuestData);
  ADD_TYPE    ("  csound                Format for CSoundData.bin", "csound", MiCSoundData);
  ADD_TYPE    ("  cspskilleffect        Format for CSpecialSkillEffectData.sbin", "cspskilleffect", MiCSpecialSkillEffectData);
  ADD_TYPE    ("  cstatus               Format for CStatusData.sbin", "cstatus", MiCStatusData);
  ADD_TYPE    ("  ctalkmessage          Format for CTalkMessageData.sbin", "ctalkmessage", MiCTalkMessageData);
  ADD_TYPE    ("  ctimeattack           Format for CTimeAttackData.sbin", "ctimeattack", MiCTimeAttackData);
  ADD_TYPE    ("  ctitle                Format for CTitleData.sbin", "ctitle", MiCTitleData);
  ADD_TYPE    ("  cultureitem           Format for CultureItemData.sbin", "cultureitem", MiCultureItemData);
  ADD_TYPE    ("  cvaluables            Format for CValuablesData.sbin", "cvaluables", MiCValuablesData);
  ADD_TYPE    ("  devilbook             Format for DevilBookData.sbin", "devilbook", MiDevilBookData);
  ADD_TYPE    ("  devilboost            Format for DevilBoostData.sbin", "devilboost", MiDevilBoostData);
  ADD_TYPE    ("  devillvluprate        Format for DevilLVUpRateData.sbin", "devillvluprate", MiDevilLVUpRateData);
  ADD_TYPE    ("  disassembly           Format for DisassemblyData.sbin", "disassembly", MiDisassemblyData);
  ADD_TYPE    ("  disassemblytrig       Format for DisassemblyTriggerData.sbin", "disassemblytrig", MiDisassemblyTriggerData);
  ADD_TYPE    ("  dynamicmap            Format for DynamicMapData.bin", "dynamicmap", MiDynamicMapData);
  ADD_TYPE    ("  enchant               Format for EnchantData.sbin", "enchant", MiEnchantData);
  ADD_TYPE    ("  equipset              Format for EquipmentSetData.sbin", "equipset", MiEquipmentSetData);
  ADD_TYPE    ("  eventdirection        Format for EventDirectionData.bin", "eventdirection", MiEventDirectionData);
  ADD_TYPE    ("  exchange              Format for ExchangeData.sbin", "exchange", MiExchangeData);
  ADD_TYPE    ("  expert                Format for ExpertClassData.sbin", "expert", MiExpertData);
  ADD_TYPE    ("  guardianassist        Format for GuardianAssistData.sbin", "guardianassist", MiGuardianAssistData);
  ADD_TYPE    ("  guardianlevel         Format for GuardianLevelData.sbin", "guardianlevel", MiGuardianLevelData);
  ADD_TYPE    ("  guardianspecial       Format for GuardianSpecialData.sbin", "guardianspecial", MiGuardianSpecialData);
  ADD_TYPE    ("  guardianunlock        Format for GuardianUnlockData.sbin", "guardianunlock", MiGuardianUnlockData);
  ADD_TYPE    ("  gvgtrophy             Format for GvGTrophyData.sbin", "gvgtrophy", MiGvGTrophyData);
  ADD_TYPE    ("  mission               Format for MissionData.sbin", "mission", MiMissionData);
  ADD_TYPE    ("  mitamabonus           Format for MitamaReunionBonusData.sbin", "mitamabonus", MiMitamaReunionBonusData);
  ADD_TYPE    ("  mitamasetbonus        Format for MitamaReunionSetBonusData.sbin", "mitamasetbonus", MiMitamaReunionSetBonusData);
  ADD_TYPE    ("  mitamaunion           Format for MitamaUnionBonusData.sbin", "mitamaunion", MiMitamaUnionBonusData);
  ADD_TYPE    ("  mod                   Format for ModificationData.sbin", "mod", MiModificationData);
  ADD_TYPE    ("  modeffect             Format for ModifiedEffectData.sbin", "modeffect", MiModifiedEffectData);
  ADD_TYPE    ("  modextrecipe          Format for ModificationExtRecipeData.sbin", "modextrecipe", MiModificationExtRecipeData);
  ADD_TYPE    ("  modtrigger            Format for ModificationTriggerData.sbin", "modtrigger", MiModificationTriggerData);
  ADD_TYPE    ("  npcbarter             Format for NPCBarterData.sbin", "npcbarter", MiNPCBarterData);
  ADD_TYPE    ("  npcbartercondition    Format for NPCBarterConditionData.sbin", "npcbartercondition", MiNPCBarterConditionData);
  ADD_TYPE    ("  npcbartergroup        Format for NPCBarterGroupData.sbin", "npcbartergroup", MiNPCBarterGroupData);
  ADD_TYPE    ("  npcbartertext         Format for NPCBarterTextData.sbin", "npcbartertext", MiNPCBarterTextData);
  ADD_TYPE    ("  npcinvisible          Format for NPCInvisibleData.sbin", "npcinvisible", MiNPCInvisibleData);
  ADD_TYPE    ("  onpc                  Format for oNPCData.sbin", "onpc", MiONPCData);
  ADD_TYPE    ("  quest                 Format for QuestData.sbin", "quest", MiQuestData);
  ADD_TYPE    ("  questbonus            Format for QuestBonusData.sbin", "questbonus", MiQuestBonusData);
  ADD_TYPE    ("  questbonuscode        Format for QuestBonusCodeData.sbin", "questbonuscode", MiQuestBonusCodeData);
  ADD_TYPE    ("  reporttype            Format for ReportTypeData.bin", "reporttype", MiReportTypeData);
  ADD_TYPE    ("  shopproduct           Format for ShopProductData.sbin", "shopproduct", MiShopProductData);
  ADD_TYPE    ("  sitem                 Format for SItemData.sbin", "sitem", MiSItemData);
  ADD_TYPE    ("  spot                  Format for SpotData.bin", "spot", MiSpotData);
  ADD_TYPE    ("  synthesis             Format for SynthesisData.sbin", "synthesis", MiSynthesisData);
  ADD_TYPE    ("  tank                  Format for TankData.sbin", "tank", MiTankData);
  ADD_TYPE    ("  timelimit             Format for TimeLimitData.sbin", "timelimit", MiTimeLimitData);
  ADD_TYPE    ("  title                 Format for CodeNameData.sbin", "title", MiTitleData);
  ADD_TYPE    ("  triunionspecial       Format for TriUnionSpecialData.sbin", "triunionspecial", MiTriUnionSpecialData);
  ADD_TYPE    ("  uiinfo                Format for UIInfoData.bin", "uiinfo", MiUIInfoData);
  ADD_TYPE    ("  warppoint             Format for WarpPointData.sbin", "warppoint", MiWarpPointData);
  ADD_TYPE_EX ("  cculture              Format for CCultureData.sbin", "cculture", MiCCultureData, GetUpperLimit());
  ADD_TYPE_EX ("  citem                 Format for CItemData.sbin", "citem", MiCItemData, GetBaseData()->GetID());
  ADD_TYPE_EX ("  ckeyitem              Format for CKeyItemData.sbin", "ckeyitem", MiCKeyItemData, GetItemData()->GetID());
  ADD_TYPE_EX ("  cmodel                Format for CModelData.sbin", "cmodel", MiCModelData, GetBase()->GetID());
  ADD_TYPE_EX ("  cskill                Format for CSkillData.bin", "cskill", MiCSkillData, GetBase()->GetID());
  ADD_TYPE_EX ("  ctransformedmodel     Format for CTransformedModelData.sbin", "ctransformedmodel", MiCTransformedModelData, GetItemID());
  ADD_TYPE_EX ("  devil                 Format for DevilData.sbin", "devil", MiDevilData, GetBasic()->GetID());
  ADD_TYPE_EX ("  devilboostextra       Format for DevilBoostExtraData.sbin", "devilboostextra", MiDevilBoostExtraData, GetStackID());
  ADD_TYPE_EX ("  devilboostitem        Format for DevilBoostItemData.sbin", "devilboostitem", MiDevilBoostItemData, GetItemID());
  ADD_TYPE_EX ("  devilboostlot         Format for DevilBoostLotData.sbin", "devilboostlot", MiDevilBoostLotData, GetLot());
  ADD_TYPE_EX ("  devilequip            Format for DevilEquipmentData.sbin", "devilequip", MiDevilEquipmentData, GetSkillID());
  ADD_TYPE_EX ("  devilequipitem        Format for DevilEquipmentItemData.sbin", "devilequipitem", MiDevilEquipmentItemData, GetItemID());
  ADD_TYPE_EX ("  devilfusion           Format for DevilFusionData.sbin", "devilfusion", MiDevilFusionData, GetSkillID());
  ADD_TYPE_EX ("  hnpc                  Format for hNPCData.sbin", "hnpc", MiHNPCData, GetBasic()->GetID());
  ADD_TYPE_EX ("  item                  Format for ItemData.sbin", "item", MiItemData, GetCommon()->GetID());
  ADD_TYPE_EX ("  skill                 Format for SkillData.sbin", "skill", MiSkillData, GetCommon()->GetID());
  ADD_TYPE_EX ("  status                Format for StatusData.sbin", "status", MiStatusData, GetCommon()->GetID());
  ADD_TYPE_EX ("  zone                  Format for ZoneData.sbin", "zone", MiZoneData, GetBasic()->GetID());
  ADD_TYPE_SEQ("  cpolygonmovie         Format for CPolygonMoveData.sbin", "cpolygonmovie", MiCPolygonMovieData);
  ADD_TYPE_SEQ("  modexteffect          Format for ModificationExtEffectData.sbin", "modexteffect", MiModificationExtEffectData);
  ADD_TYPE_SEQ("  urafieldtower         Format for UraFieldTowerData.sbin", "urafieldtower", MiUraFieldTowerData);
  ADD_TYPE_MAN("  qmp                   Format for misc qmp files", "qmp", QmpFile);
  // clang-format on

#ifdef DREAM_OBJGEN_INCLUDE_B
#include DREAM_OBJGEN_INCLUDE_B
#endif  // DREAM_OBJGEN_INCLUDE_B

  if (5 != argc) {
    return Usage(argv[0], binaryTypes);
  }

  libcomp::Log::GetSingletonPtr()->AddStandardOutputHook();

  libcomp::String mode = argv[1];
  libcomp::String bdType = argv[2];
  const char* szInPath = argv[3];
  const char* szOutPath = argv[4];

  if ("load" != mode && "save" != mode && "flatten" != mode) {
    return Usage(argv[0], binaryTypes);
  }

  libcomp::BinaryDataSet* pSet = nullptr;

  auto match = binaryTypes.find(bdType.ToUtf8());

  if (binaryTypes.end() != match) {
    pSet = (match->second.second)();
  }

  if (!pSet) {
    return Usage(argv[0], binaryTypes);
  }

  if ("qmp" == bdType && ("load" == mode || "flatten" == mode)) {
    // Manually load single record
    std::ifstream file;
    file.open(szInPath, std::ifstream::binary);

    // Read and discard magic
    uint32_t magic;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));

    if (magic != QMP_FORMAT_MAGIC) {
      std::cerr << "File magic invlalid for Qmp file: " << szInPath
                << std::endl;

      return EXIT_FAILURE;
    }

    auto qmp = std::make_shared<objects::QmpFile>();
    if (!qmp->Load(file)) {
      std::cerr << "Failed to load Qmp file: " << szInPath << std::endl;

      return EXIT_FAILURE;
    }

    ((ManualBinaryDataSet*)pSet)->AddRecord(qmp);
  }

  if ("load" == mode) {
    if ("qmp" != bdType) {
      std::ifstream file;
      file.open(szInPath, std::ifstream::binary);

      if (!pSet->Load(file)) {
        std::cerr << "Failed to load file: " << szInPath << std::endl;

        return EXIT_FAILURE;
      }
    }

    std::ofstream out;
    out.open(szOutPath);

    out << pSet->GetXml().c_str();

    if (!out.good()) {
      std::cerr << "Failed to save file: " << szOutPath << std::endl;

      return EXIT_FAILURE;
    }
  } else if ("flatten" == mode) {
    if ("qmp" != bdType) {
      std::ifstream file;
      file.open(szInPath, std::ifstream::binary);

      if (!pSet->Load(file)) {
        std::cerr << "Failed to load file: " << szInPath << std::endl;

        return EXIT_FAILURE;
      }
    }

    std::ofstream out;
    out.open(szOutPath);

    out << pSet->GetTabular().c_str();

    if (!out.good()) {
      std::cerr << "Failed to save file: " << szOutPath << std::endl;

      return EXIT_FAILURE;
    }
  } else if ("save" == mode) {
    tinyxml2::XMLDocument doc;

    if (tinyxml2::XML_SUCCESS != doc.LoadFile(szInPath)) {
      std::cerr << "Failed to parse file: " << szInPath << std::endl;

      return EXIT_FAILURE;
    }

    if (!pSet->LoadXml(doc)) {
      std::cerr << "Failed to load file: " << szInPath << std::endl;

      return EXIT_FAILURE;
    }

    std::ofstream out;
    out.open(szOutPath, std::ofstream::binary);

    if ("qmp" == bdType) {
      // Write magic
      uint32_t magic = QMP_FORMAT_MAGIC;
      out.write(reinterpret_cast<char*>(&magic), sizeof(uint32_t));

      // Write (single) entry manually
      for (auto obj : pSet->GetObjects()) {
        if (!obj->Save(out) || !out.good()) {
          std::cerr << "Failed to save QMP file: " << szOutPath << std::endl;

          return EXIT_FAILURE;
        }
      }
    } else {
      if (!pSet->Save(out)) {
        std::cerr << "Failed to save file: " << szOutPath << std::endl;

        return EXIT_FAILURE;
      }
    }
  }

#ifndef EXOTIC_PLATFORM
  // Stop the logger
  delete libcomp::Log::GetSingletonPtr();
#endif  // !EXOTIC_PLATFORM

  return EXIT_SUCCESS;
}
