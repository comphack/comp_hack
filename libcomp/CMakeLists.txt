# This file is part of COMP_hack.
#
# Copyright (C) 2010-2016 COMP_hack Team <compomega@tutanota.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(libcomp)

FIND_PACKAGE(Threads REQUIRED)

MESSAGE("** Configuring ${PROJECT_NAME} **")

# Add a target to generate API documentation.
FILE(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/api/libcomp)
GENERATE_DOCS(doc/Doxyfile.in)

# Add a directory to put the objgen output into.
FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/objgen)

SET(SYSTEM_LIBRARIES "")

IF(BSD)
FIND_PACKAGE(ExecInfo REQUIRED)

SET(SYSTEM_LIBRARIES ${SYSTEM_LIBRARIES} ${EXECINFO_LIBRARY})
ENDIF(BSD)

# Custom command to put the Git version in the @version command.
ADD_CUSTOM_TARGET(git-version COMMAND ${CMAKE_COMMAND}
    -D "SRC=${CMAKE_CURRENT_SOURCE_DIR}/src/Git.h.in"
    -D "DST=${CMAKE_CURRENT_BINARY_DIR}/Git.h"
    -P "${CMAKE_SOURCE_DIR}/cmake/git-version.cmake"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/Git.h.in"
        "${CMAKE_SOURCE_DIR}/cmake/git-version.cmake"
    BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/Git.h"
    COMMENT "Generating Git.h"
)

# Write the version information into the Constants.h file.
CONFIGURE_FILE(src/Constants.h.in "${CMAKE_CURRENT_BINARY_DIR}/Constants.h"
    ESCAPE_QUOTES @ONLY NEWLINE_STYLE UNIX)

SET(${PROJECT_NAME}_SRCS
    src/ArgumentParser.cpp
    src/BaseServer.cpp
    src/ChannelConnection.cpp
    src/Compress.cpp
    src/Convert.cpp
    src/CString.cpp
    src/Database.cpp
    src/DatabaseBind.cpp
    src/DatabaseChangeSet.cpp
    src/DatabaseMariaDB.cpp
    src/DatabaseQuery.cpp
    src/DatabaseQueryMariaDB.cpp
    src/DatabaseQuerySQLite3.cpp
    src/DatabaseSQLite3.cpp
    src/DataFile.cpp
    src/DataStore.cpp
    src/DataSyncManager.cpp
    src/Decrypt.cpp
    src/DefinitionManager.cpp
    src/DynamicObject.cpp
    src/DynamicVariable.cpp
    src/DynamicVariableFactory.cpp
    src/EncryptedConnection.cpp
    src/ErrorCodes.cpp
    src/Exception.cpp
    src/InternalConnection.cpp
    src/LobbyConnection.cpp
    src/Log.cpp
    src/ManagerPacket.cpp
    #src/MemoryFile.cpp
    src/MessageConnectionClosed.cpp
    src/MessageEncrypted.cpp
    src/MessageInit.cpp
    src/MessagePacket.cpp
    src/MessagePong.cpp
    src/MessageShutdown.cpp
    src/MessageTimeout.cpp
    src/MessageWorldNotification.cpp
    src/Object.cpp
    src/Packet.cpp
    src/PacketException.cpp
    #src/PacketScript.cpp
    src/PlatformWindows.cpp
    src/PEFile.cpp
    src/PersistentObject.cpp
    src/Randomizer.cpp
    src/ReadOnlyPacket.cpp
    src/RingBuffer.cpp
    src/ScriptEngine.cpp
    src/ServerCommandLineParser.cpp
    src/ServerConstants.cpp
    src/ServerDataManager.cpp
    src/Shutdown.cpp
    #src/Structgen.cpp
    src/TcpConnection.cpp
    src/TcpServer.cpp
    #src/ThreadManager.cpp
    src/TimerManager.cpp
    src/WindowsService.cpp
    src/Worker.cpp
)

IF(NOT WIN32)
    SET(${PROJECT_NAME}_SRCS
        ${${PROJECT_NAME}_SRCS}
        # for comp_manager and testing.
        src/Child.cpp
        src/DayCare.cpp
        src/SpawnThread.cpp
        src/WatchThread.cpp
    )
ENDIF(NOT WIN32)

IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/ConfigCustom.cpp)
    SET(${PROJECT_NAME}_SRCS ${${PROJECT_NAME}_SRCS} src/ConfigCustom.cpp)
ELSE()
    SET(${PROJECT_NAME}_SRCS ${${PROJECT_NAME}_SRCS} src/Config.cpp)
ENDIF()

# This is a list of all header files. Adding the header files here ensures they
# are listed in the source files for IDE projects.
SET(${PROJECT_NAME}_HDRS
    "${CMAKE_CURRENT_BINARY_DIR}/Constants.h"
    "${CMAKE_CURRENT_BINARY_DIR}/Git.h"

    src/ArgumentParser.h
    src/BaseServer.h
    src/ChannelConnection.h
    src/Compress.h
    src/ConnectionMessage.h
    src/Convert.h
    src/CString.h
    src/Database.h
    src/DatabaseBind.h
    src/DatabaseChangeSet.h
    src/DatabaseMariaDB.h
    src/DatabaseQuery.h
    src/DatabaseQueryMariaDB.h
    src/DatabaseQuerySQLite3.h
    src/DatabaseSQLite3.h
    src/DataFile.h
    src/DataStore.h
    src/DataSyncManager.h
    src/Decrypt.h
    src/DefinitionManager.h
    src/DynamicObject.h
    src/DynamicVariable.h
    src/DynamicVariableFactory.h
    src/DynamicVariableInt.h
    src/EncryptedConnection.h
    src/Endian.h
    src/EnumMap.h
    src/ErrorCodes.h
    src/Exception.h
    src/InternalConnection.h
    src/LobbyConnection.h
    src/Log.h
    src/Manager.h
    src/ManagerPacket.h
    #src/MemoryFile.h
    src/Message.h
    src/MessageConnectionClosed.h
    src/MessageEncrypted.h
    src/MessageExecute.h
    src/MessageInit.h
    src/MessagePacket.h
    src/MessagePong.h
    src/MessageQueue.h
    src/MessageShutdown.h
    src/MessageTick.h
    src/MessageTimeout.h
    src/MessageWorldNotification.h
    src/Object.h
    src/ObjectReference.h
    src/Packet.h
    src/PacketCodes.h
    src/PacketException.h
    src/PacketParser.h
    #src/PacketScript.h
    src/PacketStream.h
    src/PEFile.h
    src/PEFormat.h
    src/PersistentObject.h
    #src/Platform.h
    #src/PlatformLinux.h
    #src/PlatformWindows.h
    src/Randomizer.h
    src/ReadOnlyPacket.h
    src/RingBuffer.h
    src/ScriptEngine.h
    src/ServerCommandLineParser.h
    src/ServerConstants.h
    src/ServerDataManager.h
    src/Shutdown.h
    src/TcpConnection.h
    src/TcpServer.h
    #src/ThreadManager.h
    src/TimerManager.h
    src/WindowsService.h
    src/Worker.h

    # These were generated and are not worth reading.
    src/LookupTableCP1252.h
    src/LookupTableCP932.h

    # Macros to temporarily disable warnings (wrapped around 3rd party libs).
    src/PushIgnore.h
    src/PopIgnore.h
)

IF(NOT WIN32)
    SET(${PROJECT_NAME}_HDRS
        ${${PROJECT_NAME}_HDRS}
        # for comp_manager and testing.
        src/Child.h
        src/DayCare.h
        src/SpawnThread.h
        src/WatchThread.h
    )
ENDIF(NOT WIN32)

SET(${PROJECT_NAME}_SCHEMA
    schema/account.xml
    schema/accountlogin.xml
    schema/action.xml
    schema/action_addremoveitems.xml
    schema/action_addremovestatus.xml
    schema/action_createloot.xml
    schema/action_delay.xml
    schema/action_displaymessage.xml
    schema/action_grantskills.xml
    schema/action_grantxp.xml
    schema/action_homepoint.xml
    schema/action_playbgm.xml
    schema/action_playsoundeffect.xml
    schema/action_runscript.xml
    schema/action_setnpcstate.xml
    schema/action_spawn.xml
    schema/action_specialdirection.xml
    schema/action_stageeffect.xml
    schema/action_startevent.xml
    schema/action_updatecomp.xml
    schema/action_updateflag.xml
    schema/action_updatelnc.xml
    schema/action_updatepoints.xml
    schema/action_updatequest.xml
    schema/action_updatezoneflags.xml
    schema/action_zonechange.xml
    schema/action_zoneinstance.xml
    schema/character.xml
    schema/clan.xml
    schema/databaseconfig.xml
    schema/databaseconfig_mariadb.xml
    schema/databaseconfig_sqlite3.xml
    schema/demon.xml
    schema/enemy.xml
    schema/event.xml
    schema/event_direction.xml
    schema/event_message.xml
    schema/event_openmenu.xml
    schema/event_performactions.xml
    schema/event_playscene.xml
    schema/event_prompt.xml
    schema/item.xml
    schema/match.xml
    schema/packet_login.xml
    schema/packet_login_reply.xml
    schema/packet_response_code.xml
    schema/party.xml
    schema/promo.xml
    schema/qmp_file.xml
    schema/registeredchannel.xml
    schema/registeredworld.xml
    schema/reportedplayer.xml
    schema/server_command_line.xml
    schema/server_npc.xml
    schema/server_object.xml
    schema/server_shop.xml
    schema/server_zone.xml
    schema/serverconfig.xml
    schema/search.xml
    schema/spawn.xml
    schema/team.xml
    schema/test.xml
    schema/tokusei.xml
    schema/ultimatebattle.xml
    schema/webgame.xml

    # BinaryData Files
    schema/binarydata/aidata.xml
    schema/binarydata/blenddata.xml
    schema/binarydata/ceventmessagedata.xml
    schema/binarydata/chouraidata.xml
    schema/binarydata/cicondata.xml
    schema/binarydata/citemdata.xml
    schema/binarydata/cmessagedata.xml
    schema/binarydata/cmodeldata.xml
    schema/binarydata/cmodifiedeffectdata.xml
    schema/binarydata/cmultitalkdata.xml
    schema/binarydata/codenamedata.xml
    schema/binarydata/cpolygonmoviedata.xml
    schema/binarydata/cquest.xml
    schema/binarydata/csounddata.xml
    schema/binarydata/ctalkmessagedata.xml
    schema/binarydata/cultureitemdata.xml
    schema/binarydata/czonerelationdata.xml
    schema/binarydata/devilbookdata.xml
    schema/binarydata/devilboostdata.xml
    schema/binarydata/devildata.xml
    schema/binarydata/devilequipmentdata.xml
    schema/binarydata/devillvupratedata.xml
    schema/binarydata/disassemblydata.xml
    schema/binarydata/dynamicmapdata.xml
    schema/binarydata/enchantdata.xml
    schema/binarydata/equipmentsetdata.xml
    schema/binarydata/eventdirectiondata.xml
    schema/binarydata/exchangedata.xml
    schema/binarydata/expertdata.xml
    schema/binarydata/fusiondata.xml
    schema/binarydata/guardiandata.xml
    schema/binarydata/hnpcdata.xml
    schema/binarydata/itemdata.xml
    schema/binarydata/missiondata.xml
    schema/binarydata/mitamadata.xml
    schema/binarydata/modificationdata.xml
    schema/binarydata/npcbarterdata.xml
    schema/binarydata/onpcdata.xml
    schema/binarydata/questdata.xml
    schema/binarydata/shared.xml
    schema/binarydata/shopproductdata.xml
    schema/binarydata/sitemdata.xml
    schema/binarydata/skilldata.xml
    schema/binarydata/spotdata.xml
    schema/binarydata/sstatusdata.xml
    schema/binarydata/statusdata.xml
    schema/binarydata/timelimitdata.xml
    schema/binarydata/triunionspecialdata.xml
    schema/binarydata/urafieldtowerdata.xml
    schema/binarydata/warppointdata.xml
    schema/binarydata/zonedata.xml
)

SOURCE_GROUP("objgen" ${CMAKE_CURRENT_BINARY_DIR}/objgen/*)

IF(NOT USE_PREBUILT_OBJECTS)
    OBJGEN_XML(${PROJECT_NAME}_STRUCTS
        # Main schema
        schema/master.xml

        # Included schemas
        ${${PROJECT_NAME}_SCHEMA}

        # Include paths
        schema

        # Output files
        Account.h
        Account.cpp
        AccountLogin.h
        AccountLogin.cpp
        AccountWorldData.h
        AccountWorldData.cpp
        Action.h
        Action.cpp
        ActionAddRemoveItems.h
        ActionAddRemoveItems.cpp
        ActionAddRemoveStatus.h
        ActionAddRemoveStatus.cpp
        ActionCreateLoot.h
        ActionCreateLoot.cpp
        ActionDelay.h
        ActionDelay.cpp
        ActionDisplayMessage.h
        ActionDisplayMessage.cpp
        ActionGrantSkills.h
        ActionGrantSkills.cpp
        ActionGrantXP.h
        ActionGrantXP.cpp
        ActionPlayBGM.h
        ActionPlayBGM.cpp
        ActionPlaySoundEffect.h
        ActionPlaySoundEffect.cpp
        ActionRunScript.h
        ActionRunScript.cpp
        ActionSetHomepoint.h
        ActionSetHomepoint.cpp
        ActionSetNPCState.h
        ActionSetNPCState.cpp
        ActionSpawn.h
        ActionSpawn.cpp
        ActionSpecialDirection.h
        ActionSpecialDirection.cpp
        ActionStageEffect.h
        ActionStageEffect.cpp
        ActionStartEvent.h
        ActionStartEvent.cpp
        ActionUpdateCOMP.h
        ActionUpdateCOMP.cpp
        ActionUpdateFlag.h
        ActionUpdateFlag.cpp
        ActionUpdateLNC.h
        ActionUpdateLNC.cpp
        ActionUpdatePoints.h
        ActionUpdatePoints.cpp
        ActionUpdateQuest.h
        ActionUpdateQuest.cpp
        ActionUpdateZoneFlags.h
        ActionUpdateZoneFlags.cpp
        ActionZoneInstance.h
        ActionZoneInstance.cpp
        ActionZoneChange.h
        ActionZoneChange.cpp
        BazaarData.h
        BazaarData.cpp
        BazaarItem.h
        BazaarItem.cpp
        Clan.h
        Clan.cpp
        ClanInfo.h
        ClanInfo.cpp
        ClanMember.h
        ClanMember.cpp
        ChannelLogin.h
        ChannelLogin.cpp
        CharacterLogin.h
        CharacterLogin.cpp
        CharacterProgress.h
        CharacterProgress.cpp
        CultureData.h
        CultureData.cpp
        PvPData.h
        PvPData.cpp
        DatabaseConfig.h
        DatabaseConfig.cpp
        DatabaseConfigMariaDB.h
        DatabaseConfigMariaDB.cpp
        DatabaseConfigSQLite3.h
        DatabaseConfigSQLite3.cpp
        EntityStats.h
        EntityStats.cpp
        EventCounter.h
        EventCounter.cpp
        Item.h
        Item.cpp
        Demon.h
        Demon.cpp
        Quest.h
        Quest.cpp
        DemonQuest.h
        DemonQuest.cpp
        Character.h
        Character.cpp
        DemonBox.h
        DemonBox.cpp
        DemonPresent.h
        DemonPresent.cpp
        DemonQuestReward.h
        DemonQuestReward.cpp
        DropSet.h
        DropSet.cpp
        EnemyBase.h
        EnemyBase.cpp
        Enemy.h
        Enemy.cpp
        Ally.h
        Ally.cpp
        EventConditionData.cpp
        EventConditionData.h
        EventCondition.h
        EventCondition.cpp
        EventBase.h
        EventBase.cpp
        EventSequence.h
        EventSequence.cpp
        Event.h
        Event.cpp
        EventChoice.h
        EventChoice.cpp
        EventDirection.h
        EventDirection.cpp
        EventExNPCMessage.h
        EventExNPCMessage.cpp
        EventFlagCondition.h
        EventFlagCondition.cpp
        EventITime.h
        EventITime.cpp
        EventMultitalk.h
        EventMultitalk.cpp
        EventNPCMessage.h
        EventNPCMessage.cpp
        EventOpenMenu.h
        EventOpenMenu.cpp
        EventPerformActions.h
        EventPerformActions.cpp
        EventPlayScene.h
        EventPlayScene.cpp
        EventPrompt.h
        EventPrompt.cpp
        EventScriptCondition.h
        EventScriptCondition.cpp
        Expertise.h
        Expertise.cpp
        FriendSettings.h
        FriendSettings.cpp
        Hotbar.h
        Hotbar.cpp
        InheritedSkill.h
        InheritedSkill.cpp
        InstanceAccess.h
        InstanceAccess.cpp
        ItemBox.h
        ItemBox.cpp
        ItemDrop.h
        ItemDrop.cpp
        Match.h
        Match.cpp
        MatchEntry.h
        MatchEntry.cpp
        ObjectPosition.h
        ObjectPosition.cpp
        PacketLogin.h
        PacketLogin.cpp
        PacketLoginReply.h
        PacketLoginReply.cpp
        PacketResponseCode.h
        PacketResponseCode.cpp
        Party.h
        Party.cpp
        PartyCharacter.h
        PartyCharacter.cpp
        PartyMember.h
        PartyMember.cpp
        PentalphaEntry.h
        PentalphaEntry.cpp
        PentalphaMatch.h
        PentalphaMatch.cpp
        PlasmaSpawn.h
        PlasmaSpawn.cpp
        PostItem.h
        PostItem.cpp
        Promo.h
        Promo.cpp
        PromoExchange.h
        PromoExchange.cpp
        PvPMatch.h
        PvPMatch.cpp
        QmpFile.h
        QmpFile.cpp
        QmpElement.h
        QmpElement.cpp
        QmpBoundary.h
        QmpBoundary.cpp
        QmpBoundaryLine.h
        QmpBoundaryLine.cpp
        QmpNavPoint.h
        QmpNavPoint.cpp
        RegisteredChannel.h
        RegisteredChannel.cpp
        RegisteredWorld.h
        RegisteredWorld.cpp
        ReportedPlayer.h
        ReportedPlayer.cpp
        SearchEntry.h
        SearchEntry.cpp
        ServerBazaar.h
        ServerBazaar.cpp
        ServerCommandLine.h
        ServerCommandLine.cpp
        ServerConfig.h
        ServerConfig.cpp
        ServerCultureMachineSet.h
        ServerCultureMachineSet.cpp
        ServerNPC.h
        ServerNPC.cpp
        ServerObject.h
        ServerObject.cpp
        ServerObjectBase.h
        ServerObjectBase.cpp
        ServerShop.h
        ServerShop.cpp
        ServerShopProduct.h
        ServerShopProduct.cpp
        ServerShopTab.h
        ServerShopTab.cpp
        ServerZone.h
        ServerZone.cpp
        ServerZoneInstance.h
        ServerZoneInstance.cpp
        ServerZoneInstanceVariant.h
        ServerZoneInstanceVariant.cpp
        PvPInstanceVariant.h
        PvPInstanceVariant.cpp
        ServerZonePartial.h
        ServerZonePartial.cpp
        ServerZoneSpot.h
        ServerZoneSpot.cpp
        ServerZoneTrigger.h
        ServerZoneTrigger.cpp
        StatusEffect.h
        StatusEffect.cpp
        Spawn.h
        Spawn.cpp
        SpawnGroup.h
        SpawnGroup.cpp
        SpawnLocation.h
        SpawnLocation.cpp
        SpawnLocationGroup.h
        SpawnLocationGroup.cpp
        SpawnRestriction.h
        SpawnRestriction.cpp
        Team.h
        Team.cpp
        TestObject.h
        TestObject.cpp
        TestObjectA.h
        TestObjectA.cpp
        TestObjectB.h
        TestObjectB.cpp
        TestObjectC.h
        TestObjectC.cpp
        TestObjectD.h
        TestObjectD.cpp
        Tokusei.h
        Tokusei.cpp
        TokuseiAspect.h
        TokuseiAspect.cpp
        TokuseiAttributes.h
        TokuseiAttributes.cpp
        TokuseiCondition.h
        TokuseiCondition.cpp
        TokuseiCorrectTbl.h
        TokuseiCorrectTbl.cpp
        TokuseiSkillCondition.h
        TokuseiSkillCondition.cpp
        UBTournament.h
        UBTournament.cpp
        UBResult.h
        UBResult.cpp
        WebGameSession.h
        WebGameSession.cpp
        WorldSharedConfig.h
        WorldSharedConfig.cpp

        # BinaryData Files
        MiAcquisitionData.cpp
        MiAcquisitionData.h
        MiAcquisitionSkillData.cpp
        MiAcquisitionSkillData.h
        MiAddStatusTbl.cpp
        MiAddStatusTbl.h
        MiAIData.cpp
        MiAIData.h
        MiAIRelationData.cpp
        MiAIRelationData.h
        MiBattleDamageData.cpp
        MiBattleDamageData.h
        MiBlendData.cpp
        MiBlendData.h
        MiBlendData_Item.cpp
        MiBlendData_Item.h
        MiBlendExtData.cpp
        MiBlendExtData.h
        MiBlendExtData_DstItemChange.cpp
        MiBlendExtData_DstItemChange.h
        MiBlendExtData_SrcItemChange.cpp
        MiBlendExtData_SrcItemChange.h
        MiBreakData.cpp
        MiBreakData.h
        MiCancelData.cpp
        MiCancelData.h
        MiCastBasicData.cpp
        MiCastBasicData.h
        MiCastCancelData.cpp
        MiCastCancelData.h
        MiCastData.cpp
        MiCastData.h
        MiCategoryData.h
        MiCategoryData.cpp
        MiCEventMessageData.cpp
        MiCEventMessageData.h
        MiCHouraiData.cpp
        MiCHouraiData.h
        MiCItemBaseData.cpp
        MiCItemBaseData.h
        MiCItemData.cpp
        MiCItemData.h
        MiCItemMotionData.cpp
        MiCItemMotionData.h
        MiCItemSPEffectData.cpp
        MiCItemSPEffectData.h
        MiMultiTalkCmdTbl.cpp
        MiMultiTalkCmdTbl.h
        MiCMultiTalkData.cpp
        MiCMultiTalkData.h
        MiCMessageData.cpp
        MiCMessageData.h
        MiCPolygonMovieData.cpp
        MiCPolygonMovieData.h
        MiCultureItemData.cpp
        MiCultureItemData.h
        MiPMBaseInfo.cpp
        MiPMBaseInfo.h
        MiPMCameraKeyTbl.cpp
        MiPMCameraKeyTbl.h
        MiPMMsgKeyTbl.cpp
        MiPMMsgKeyTbl.h
        MiPMBGMKeyTbl.cpp
        MiPMBGMKeyTbl.h
        MiPMSEKeyTbl.cpp
        MiPMSEKeyTbl.h
        MiPMEffectKeyTbl.cpp
        MiPMEffectKeyTbl.h
        MiPMFadeKeyTbl.cpp
        MiPMFadeKeyTbl.h
        MiPMGouraudKeyTbl.cpp
        MiPMGouraudKeyTbl.h
        MiPMFogKeyTbl.cpp
        MiPMFogKeyTbl.h
        MiPMScalingHelperTbl.cpp
        MiPMScalingHelperTbl.h
        MiPMAttachCharacterTbl.cpp
        MiPMAttachCharacterTbl.h
        MiPMMotionKeyTbl.cpp
        MiPMMotionKeyTbl.h
        MiConditionData.cpp
        MiConditionData.h
        MiCorrectTbl.h
        MiCorrectTbl.cpp
        MiCostTbl.cpp
        MiCostTbl.h
        MiCQuestData.cpp
        MiCQuestData.h
        MiDamageData.cpp
        MiDamageData.h
        MiDCategoryData.cpp
        MiDCategoryData.h
        MiDevilBattleData.cpp
        MiDevilBattleData.h
        MiDevilBookData.cpp
        MiDevilBookData.h
        MiDevilBoostData.cpp
        MiDevilBoostData.h
        MiDevilBoostExtraData.cpp
        MiDevilBoostExtraData.h
        MiDevilBoostItemData.cpp
        MiDevilBoostItemData.h
        MiDevilBoostLotData.cpp
        MiDevilBoostLotData.h
        MiDevilBoostRequirementData.cpp
        MiDevilBoostRequirementData.h
        MiDevilBoostResultData.cpp
        MiDevilBoostResultData.h
        MiDevilCrystalData.h
        MiDevilCrystalData.cpp
        MiDevilData.h
        MiDevilData.cpp
        MiDevilEquipmentData.h
        MiDevilEquipmentData.cpp
        MiDevilEquipmentItemData.h
        MiDevilEquipmentItemData.cpp
        MiDevilFamiliarityData.cpp
        MiDevilFamiliarityData.h
        MiDevilFusionData.h
        MiDevilFusionData.cpp
        MiDevilLVUpData.cpp
        MiDevilLVUpData.h
        MiDevilLVUpRateData.cpp
        MiDevilLVUpRateData.h
        MiDevilReunionConditionData.cpp
        MiDevilReunionConditionData.h
        MiDisassemblyData.cpp
        MiDisassemblyData.h
        MiDisassemblyMaterialData.cpp
        MiDisassemblyMaterialData.h
        MiDisassemblyTriggerData.cpp
        MiDisassemblyTriggerData.h
        MiDischargeData.cpp
        MiDischargeData.h
        MiDoTDamageData.cpp
        MiDoTDamageData.h
        MiDynamicMapData.h
        MiDynamicMapData.cpp
        MiEffectData.cpp
        MiEffectData.h
        MiEffectiveRangeData.cpp
        MiEffectiveRangeData.h
        MiEnchantCharasticData.h
        MiEnchantCharasticData.cpp
        MiEnchantData.h
        MiEnchantData.cpp
        EnchantSetData.h
        EnchantSetData.cpp
        EnchantSpecialData.h
        EnchantSpecialData.cpp
        MiEquipmentSetData.cpp
        MiEquipmentSetData.h
        MiExchangeData.cpp
        MiExchangeData.h
        MiExchangeObjectData.cpp
        MiExchangeObjectData.h
        MiExchangeOptionData.cpp
        MiExchangeOptionData.h
        MiExpertChainData.cpp
        MiExpertChainData.h
        MiExpertClassData.cpp
        MiExpertClassData.h
        MiExpertData.cpp
        MiExpertData.h
        MiExpertGrowthTbl.cpp
        MiExpertGrowthTbl.h
        MiExpertRankData.cpp
        MiExpertRankData.h
        MiFindInfo.cpp
        MiFindInfo.h
        MiGrowthData.cpp
        MiGrowthData.h
        MiGuardianAssistData.cpp
        MiGuardianAssistData.h
        MiGuardianLevelData.cpp
        MiGuardianLevelData.h
        MiGuardianLevelDataEntry.cpp
        MiGuardianLevelDataEntry.h
        MiGuardianSpecialData.cpp
        MiGuardianSpecialData.h
        MiGuardianUnlockData.cpp
        MiGuardianUnlockData.h
        MiHNPCAppearanceData.cpp
        MiHNPCAppearanceData.h
        MiHNPCBasicData.cpp
        MiHNPCBasicData.h
        MiHNPCData.cpp
        MiHNPCData.h
        MiItemBasicData.h
        MiItemBasicData.cpp
        MiItemPvPData.h
        MiItemPvPData.cpp
        MiKnockBackData.cpp
        MiKnockBackData.h
        MiMissionData.h
        MiMissionData.cpp
        MiMissionExit.h
        MiMissionExit.cpp
        MiMitamaReunionBonusData.cpp
        MiMitamaReunionBonusData.h
        MiMitamaReunionSetBonusData.cpp
        MiMitamaReunionSetBonusData.h
        MiMitamaUnionBonusData.cpp
        MiMitamaUnionBonusData.h
        MiModificationData.cpp
        MiModificationData.h
        MiModificationExtEffectData.cpp
        MiModificationExtEffectData.h
        MiModificationExtRecipeData.cpp
        MiModificationExtRecipeData.h
        MiModificationTriggerData.cpp
        MiModificationTriggerData.h
        MiModifiedEffectData.cpp
        MiModifiedEffectData.h
        MiNegotiationDamageData.cpp
        MiNegotiationDamageData.h
        MiNegotiationData.cpp
        MiNegotiationData.h
        MiNextEpisodeInfo.cpp
        MiNextEpisodeInfo.h
        MiNPCBarterData.cpp
        MiNPCBarterData.h
        MiNPCBarterItemData.cpp
        MiNPCBarterItemData.h
        MiNPCBasicData.cpp
        MiNPCBasicData.h
        MiONPCData.cpp
        MiONPCData.h
        MiPossessionData.h
        MiPossessionData.cpp
        MiQuestBonusCodeData.h
        MiQuestBonusCodeData.cpp
        MiQuestData.h
        MiQuestData.cpp
        MiQuestPhaseData.h
        MiQuestPhaseData.cpp
        QuestPhaseRequirement.h
        QuestPhaseRequirement.cpp
        MiQuestUpperCondition.h
        MiQuestUpperCondition.cpp
        MiRentalData.h
        MiRentalData.cpp
        MiRestrictionData.cpp
        MiRestrictionData.h
        MiRelationZoneIDData.h
        MiRelationZoneIDData.cpp
        MiCZoneRelationData.h
        MiCZoneRelationData.cpp
        MiShopProductData.cpp
        MiShopProductData.h
        MiSItemData.h
        MiSItemData.cpp
        MiSkillBasicData.cpp
        MiSkillBasicData.h
        MiSkillCharasticData.cpp
        MiSkillCharasticData.h
        MiSkillData.cpp
        MiSkillData.h
        MiSkillItemStatusCommonData.h
        MiSkillItemStatusCommonData.cpp
        MiSkillPvPData.cpp
        MiSkillPvPData.h
        MiSkillSpecialParams.cpp
        MiSkillSpecialParams.h
        MiSkillTbl.h
        MiSkillTbl.cpp
        MiSpecialConditionData.h
        MiSpecialConditionData.cpp
        MiSStatusData.cpp
        MiSStatusData.h
        MiStatusBasicData.cpp
        MiStatusBasicData.h
        MiStatusData.cpp
        MiStatusData.h
        MiSummonData.cpp
        MiSummonData.h
        MiSynthesisData.cpp
        MiSynthesisData.h
        MiSynthesisItemData.cpp
        MiSynthesisItemData.h
        MiCTalkMessageData.cpp
        MiCTalkMessageData.h
        MiTargetData.cpp
        MiTargetData.h
        MiTimeLimitData.h
        MiTimeLimitData.cpp
        MiTitleData.h
        MiTitleData.cpp
        MiTriUnionSpecialData.cpp
        MiTriUnionSpecialData.h
        MiUnionData.cpp
        MiUnionData.h
        MiUraFieldTowerData.h
        MiUraFieldTowerData.cpp
        MiUseRestrictionsData.h
        MiUseRestrictionsData.cpp
        MiSpotData.h
        MiSpotData.cpp
        MiWarpPointData.cpp
        MiWarpPointData.h
        MiZoneBasicData.h
        MiZoneBasicData.cpp
        MiZoneBGMData.h
        MiZoneBGMData.cpp
        MiZoneCameraData.h
        MiZoneCameraData.cpp
        MiZoneFileData.h
        MiZoneFileData.cpp
        MiZoneFogData.h
        MiZoneFogData.cpp
        MiZoneGouraudData.h
        MiZoneGouraudData.cpp
        MiZoneLensFlareLayerData.h
        MiZoneLensFlareLayerData.cpp
        MiZoneLensFlareData.h
        MiZoneLensFlareData.cpp
        MiZoneOtherData.h
        MiZoneOtherData.cpp
        MiZoneSkyData.h
        MiZoneSkyData.cpp
        MiZoneData.h
        MiZoneData.cpp
        MiCModifiedEffectData.h
        MiCModifiedEffectData.cpp
        MiCModelData.h
        MiCModelData.cpp
        MiCModelBase.h
        MiCModelBase.cpp
        MiCModelView.h
        MiCModelView.cpp
        MiCModelMotionMap.h
        MiCModelMotionMap.cpp
        MiCIconData.h
        MiCIconData.cpp
        MiItemData.h
        MiItemData.cpp
        MiCSoundData.h
        MiCSoundData.cpp
        MiEventDirectionData.h
        MiEventDirectionData.cpp
    )
ENDIF(NOT USE_PREBUILT_OBJECTS)

COVERALLS_SOURCES(${${PROJECT_NAME}_SRCS})

ADD_LIBRARY(comp STATIC ${${PROJECT_NAME}_SRCS} ${${PROJECT_NAME}_HDRS}
    ${${PROJECT_NAME}_MOC_SRCS} ${${PROJECT_NAME}_STRUCTS})

TARGET_COMPILE_DEFINITIONS(comp PUBLIC ${SQRAT_DEFINES})

SET_TARGET_PROPERTIES(comp PROPERTIES FOLDER "Libraries")

IF(SYSTEMD_FOUND)
    SET(SYSTEMD_INCLUDES ${SYSTEMD_INCLUDE_DIRS})
ENDIF(SYSTEMD_FOUND)

TARGET_INCLUDE_DIRECTORIES(comp PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
    ${LIBCOMP_INCLUDES}
    ${ASIO_INCLUDE_DIRS}
    ${SYSTEMD_INCLUDES}
    ${SQRAT_INCLUDE_DIRS}
)

IF(SYSTEMD_FOUND)
    SET(SYSTEMD_LIBS ${SYSTEMD_LIBRARIES})
ENDIF(SYSTEMD_FOUND)

IF(USE_PREBUILT_OBJECTS)
    SET(LIBOBJECTS_LIB libobjects)
ENDIF(USE_PREBUILT_OBJECTS)

TARGET_LINK_LIBRARIES(comp ${LIBOBJECTS_LIB} mariadbclient ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES}
    ${SYSTEM_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} objgen sqlite3
    squirrel sqstdlib ttvfs ttvfs_zip tinyxml2 physfs ${SYSTEMD_LIBS})

ADD_DEPENDENCIES(comp asio gsl git-version)

IF(WIN32)
    TARGET_LINK_LIBRARIES(comp shlwapi advapi32 iphlpapi psapi shell32
        userenv ws2_32 dbghelp)
ENDIF(WIN32)

IF(USE_COTIRE)
    cotire(comp)
ENDIF(USE_COTIRE)

# List of unit tests to add to CTest.
SET(${PROJECT_NAME}_TEST_SRCS
    Convert
    Decrypt

    # This test can take too long so disable it for now.
    # DiffieHellman

    GeneratedObjects
    MariaDB
    Packet
    ScriptEngine
    String
    VectorStream
    #XmlUtils
)

IF(NOT BSD)
    # Add the unit tests.
    CREATE_GTESTS(LIBS ${LIBOBJECTS_LIB} comp
        SRCS ${${PROJECT_NAME}_TEST_SRCS})
ENDIF(NOT BSD)
