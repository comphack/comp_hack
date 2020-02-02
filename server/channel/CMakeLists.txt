# This file is part of COMP_hack.
#
# Copyright (C) 2010-2020 COMP_hack Team <compomega@tutanota.com>
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

# Add a target to generate API documentation.
FILE(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/api/server/channel)
GENERATE_DOCS_EX(doc/Doxyfile.in comp_channel)

IF(IMPORT_CHANNEL)

INSTALL(FILES "${CMAKE_BINARY_DIR}/bin/comp_channel${CMAKE_EXECUTABLE_SUFFIX}"
    DESTINATION ${COMP_INSTALL_DIR} COMPONENT channel)

# Include the PDB file if on Windows
IF(WIN32)
    INSTALL(FILES "${CMAKE_BINARY_DIR}/bin/comp_channel.pdb"
        DESTINATION ${COMP_INSTALL_DIR} COMPONENT channel)
ENDIF(WIN32)

ELSE(IMPORT_CHANNEL)

PROJECT(comp_channel)

MESSAGE("** Configuring ${PROJECT_NAME} **")

# Add a directory to put the objgen output into.
FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/objgen)

SET(${PROJECT_NAME}_SRCS
    ${CMAKE_SOURCE_DIR}/libcomp/libcomp/src/WindowsServiceMain.cpp

    src/AccountManager.cpp
    src/ActionManager.cpp
    src/ActiveEntityState.cpp
    src/AICommand.cpp
    src/AIManager.cpp
    src/AIState.cpp
    src/AllyState.cpp
    src/BazaarState.cpp
    src/ChannelClientConnection.cpp
    src/ChannelServer.cpp
    src/ChannelSyncManager.cpp
    src/CharacterManager.cpp
    src/ChatManager.cpp
    src/CharacterState.cpp
    src/ClientState.cpp
    src/CultureMachineState.cpp
    src/DemonState.cpp
    src/EnemyState.cpp
    src/EntityState.cpp
    src/EventManager.cpp
    src/FusionManager.cpp
    src/FusionTables.cpp
    src/ManagerClientPacket.cpp
    src/ManagerConnection.cpp
    src/ManagerSystem.cpp
    src/MatchManager.cpp
    src/PerformanceTimer.cpp
    src/PlasmaState.cpp
    src/SkillManager.cpp
    src/TokuseiManager.cpp
    src/WorldClock.cpp
    src/Zone.cpp
    src/ZoneInstance.cpp
    src/ZoneGeometry.cpp
    src/ZoneGeometryLoader.cpp
    src/ZoneManager.cpp
    src/main.cpp
)

SET(${PROJECT_NAME}_HDRS
    src/AccountManager.h
    src/ActionManager.h
    src/ActiveEntityState.h
    src/AICommand.h
    src/AIManager.h
    src/AIState.h
    src/AllyState.h
    src/BazaarState.h
    src/ChannelClientConnection.h
    src/ChannelServer.h
    src/ChannelSyncManager.h
    src/CharacterManager.h
    src/ChatManager.h
    src/CharacterState.h
    src/ClientState.h
    src/CultureMachineState.h
    src/DemonState.h
    src/EnemyState.h
    src/EntityState.h
    src/EventManager.h
    src/FusionManager.h
    src/FusionTables.h
    src/ManagerClientPacket.h
    src/ManagerConnection.h
    src/ManagerSystem.h
    src/MatchManager.h
    src/Packets.h
    src/PerformanceTimer.h
    src/PlasmaState.h
    src/SkillManager.h
    src/TokuseiManager.h
    src/WorldClock.h
    src/Zone.h
    src/ZoneInstance.h
    src/ZoneGeometry.h
    src/ZoneGeometryLoader.h
    src/ZoneManager.h
)

SET(${PROJECT_NAME}_SCHEMA
    schema/aistate.xml
    schema/channelconfig.xml
    schema/clientstate.xml
    schema/entitystate.xml
    schema/loot.xml
    schema/zone.xml
)

SOURCE_GROUP("objgen" ${CMAKE_CURRENT_BINARY_DIR}/objgen/*)

OBJGEN_XML(${PROJECT_NAME}_STRUCTS
    # Main schema
    schema/master.xml

    # Included schemas
    ${${PROJECT_NAME}_SCHEMA}

    # Include paths
    schema
    ../../libcomp/libcomp/schema

    # Output files
    AIStateObject.h
    AIStateObject.cpp
    EntityStateObject.h
    EntityStateObject.cpp
    ActivatedAbility.h
    ActivatedAbility.cpp
    CalculatedEntityState.h
    CalculatedEntityState.cpp
    ActiveEntityStateObject.h
    ActiveEntityStateObject.cpp
    ChannelConfig.h
    ChannelConfig.cpp
    ClientCostAdjustment.h
    ClientCostAdjustment.cpp
    ClientStateObject.h
    ClientStateObject.cpp
    DiasporaBase.h
    DiasporaBase.cpp
    DigitalizeState.h
    DigitalizeState.cpp
    EventInstance.h
    EventInstance.cpp
    EventState.h
    EventState.cpp
    Loot.h
    Loot.cpp
    LootBox.h
    LootBox.cpp
    DestinyBox.h
    DestinyBox.cpp
    PlayerExchangeSession.h
    PlayerExchangeSession.cpp
    PvPBase.h
    PvPBase.cpp
    PvPPlayerStats.h
    PvPPlayerStats.cpp
    PvPInstanceStats.h
    PvPInstanceStats.cpp
    TriFusionHostSession.h
    TriFusionHostSession.cpp
    UBMatch.h
    UBMatch.cpp
    ZoneInstanceObject.h
    ZoneInstanceObject.cpp
    ZoneObject.h
    ZoneObject.cpp
)

SOURCE_GROUP("Source Files\\Packets\\Amala" src/packets/amala/*)
SOURCE_GROUP("Source Files\\Packets\\Game" src/packets/game/*)
SOURCE_GROUP("Source Files\\Packets\\Internal" src/packets/internal/*)

SET(${PROJECT_NAME}_PACKETS
    src/packets/game/Unsupported.cpp
    src/packets/game/Login.cpp                  # 0x0000
    src/packets/game/Auth.cpp                   # 0x0002
    src/packets/game/SendData.cpp               # 0x0004
    src/packets/game/Logout.cpp                 # 0x0005
    src/packets/game/PopulateZone.cpp           # 0x0019
    src/packets/game/Move.cpp                   # 0x001C
    src/packets/game/Pivot.cpp                  # 0x0020
    src/packets/game/Chat.cpp                   # 0x0026
    src/packets/game/Tell.cpp                   # 0x0027
    src/packets/game/SkillActivate.cpp          # 0x0030
    src/packets/game/SkillExecute.cpp           # 0x0031
    src/packets/game/SkillCancel.cpp            # 0x0032
    src/packets/game/SkillTarget.cpp            # 0x003D
    src/packets/game/ExpertiseDown.cpp          # 0x0042
    src/packets/game/AllocateSkillPoint.cpp     # 0x0049
    src/packets/game/ToggleExpertise.cpp        # 0x004F
    src/packets/game/LearnSkill.cpp             # 0x0051
    src/packets/game/DemonSkillUpdate.cpp       # 0x0054
    src/packets/game/KeepAlive.cpp              # 0x0056
    src/packets/game/FixObjectPosition.cpp      # 0x0058
    src/packets/game/State.cpp                  # 0x005A
    src/packets/game/DemonData.cpp              # 0x005B
    src/packets/game/DemonBox.cpp               # 0x005C
    src/packets/game/DemonBoxData.cpp           # 0x005E
    src/packets/game/ChannelList.cpp            # 0x0063
    src/packets/game/ReviveCharacter.cpp        # 0x0067
    src/packets/game/StopMovement.cpp           # 0x006F
    src/packets/game/SpotTriggered.cpp          # 0x0071
    src/packets/game/WorldTime.cpp              # 0x0072
    src/packets/game/ItemBox.cpp                # 0x0074
    src/packets/game/ItemMove.cpp               # 0x0076
    src/packets/game/ItemDrop.cpp               # 0x0077
    src/packets/game/ItemStack.cpp              # 0x0078
    src/packets/game/EquipmentList.cpp          # 0x007B
    src/packets/game/TradeRequest.cpp           # 0x007D
    src/packets/game/TradeAccept.cpp            # 0x0080
    src/packets/game/TradeAddItem.cpp           # 0x0082
    src/packets/game/TradeLock.cpp              # 0x0085
    src/packets/game/TradeFinish.cpp            # 0x0088
    src/packets/game/TradeCancel.cpp            # 0x008B
    src/packets/game/LootItem.cpp               # 0x008E
    src/packets/game/CashBalance.cpp            # 0x0090
    src/packets/game/ShopData.cpp               # 0x0092
    src/packets/game/ShopBuy.cpp                # 0x0094
    src/packets/game/ShopSell.cpp               # 0x0096
    src/packets/game/DemonBoxMove.cpp           # 0x009A
    src/packets/game/DemonDismiss.cpp           # 0x009B
    src/packets/game/PostList.cpp               # 0x009C
    src/packets/game/PostItem.cpp               # 0x009E
    src/packets/game/PostGift.cpp               # 0x00A0
    src/packets/game/HotbarData.cpp             # 0x00A2
    src/packets/game/HotbarSave.cpp             # 0x00A4
    src/packets/game/EventResponse.cpp          # 0x00B7
    src/packets/game/ValuableList.cpp           # 0x00B8
    src/packets/game/ObjectInteraction.cpp      # 0x00BA
    src/packets/game/FriendInfo.cpp             # 0x00BD
    src/packets/game/FriendRequest.cpp          # 0x00C0
    src/packets/game/FriendAddRemove.cpp        # 0x00C3
    src/packets/game/FriendData.cpp             # 0x00C6
    src/packets/game/PartyInvite.cpp            # 0x00D2
    src/packets/game/PartyJoin.cpp              # 0x00D5
    src/packets/game/PartyCancel.cpp            # 0x00D8
    src/packets/game/PartyLeave.cpp             # 0x00DA
    src/packets/game/PartyDisband.cpp           # 0x00DD
    src/packets/game/PartyLeaderUpdate.cpp      # 0x00E0
    src/packets/game/PartyDropRule.cpp          # 0x00E3
    src/packets/game/PartyKick.cpp              # 0x00EB
    src/packets/game/DemonFusion.cpp            # 0x00EF
    src/packets/game/LootDemonEggData.cpp       # 0x00F1
    src/packets/game/Sync.cpp                   # 0x00F3
    src/packets/game/ShopRepair.cpp             # 0x00F6
    src/packets/game/Rotate.cpp                 # 0x00F8
    src/packets/game/LootBossBox.cpp            # 0x00FC
    src/packets/game/UnionFlag.cpp              # 0x0100
    src/packets/game/ItemDepoList.cpp           # 0x0102
    src/packets/game/DepoRent.cpp               # 0x0104
    src/packets/game/LootTreasureBox.cpp        # 0x0107
    src/packets/game/QuestActiveList.cpp        # 0x010B
    src/packets/game/QuestCompletedList.cpp     # 0x010C
    src/packets/game/BazaarMarketOpen.cpp       # 0x0113
    src/packets/game/BazaarMarketClose.cpp      # 0x0115
    src/packets/game/BazaarMarketInfo.cpp       # 0x0117
    src/packets/game/BazaarItemAdd.cpp          # 0x0119
    src/packets/game/BazaarItemDrop.cpp         # 0x011B
    src/packets/game/BazaarItemUpdate.cpp       # 0x011D
    src/packets/game/BazaarItemBuy.cpp          # 0x011F
    src/packets/game/BazaarMarketSales.cpp      # 0x0122
    src/packets/game/SearchEntrySelf.cpp        # 0x0128
    src/packets/game/SearchList.cpp             # 0x012A
    src/packets/game/SearchEntryData.cpp        # 0x012C
    src/packets/game/SearchEntryRegister.cpp    # 0x012E
    src/packets/game/SearchEntryUpdate.cpp      # 0x0130
    src/packets/game/SearchEntryRemove.cpp      # 0x0132
    src/packets/game/SearchAppReply.cpp         # 0x0135
    src/packets/game/ClanDisband.cpp            # 0x0138
    src/packets/game/ClanInvite.cpp             # 0x013B
    src/packets/game/ClanJoin.cpp               # 0x013E
    src/packets/game/ClanCancel.cpp             # 0x0140
    src/packets/game/ClanKick.cpp               # 0x0142
    src/packets/game/ClanMasterUpdate.cpp       # 0x0145
    src/packets/game/ClanSubMasterUpdate.cpp    # 0x0148
    src/packets/game/ClanLeave.cpp              # 0x014B
    src/packets/game/ClanChat.cpp               # 0x014E
    src/packets/game/ClanInfo.cpp               # 0x0150
    src/packets/game/ClanList.cpp               # 0x0152
    src/packets/game/ClanData.cpp               # 0x0154
    src/packets/game/ClanForm.cpp               # 0x0156
    src/packets/game/ItemPrice.cpp              # 0x0157
    src/packets/game/BazaarState.cpp            # 0x015F
    src/packets/game/BazaarClerkSet.cpp         # 0x0161
    src/packets/game/BazaarPrice.cpp            # 0x0164
    src/packets/game/BazaarMarketInfoSelf.cpp   # 0x0166
    src/packets/game/Warp.cpp                   # 0x016A
    src/packets/game/SkillExecuteInstant.cpp    # 0x016C
    src/packets/game/SyncCharacter.cpp          # 0x017E
    src/packets/game/DemonAISet.cpp             # 0x0181
    src/packets/game/BazaarInteract.cpp         # 0x0184
    src/packets/game/SkillForget.cpp            # 0x0186
    src/packets/game/BazaarMarketEnd.cpp        # 0x0188
    src/packets/game/BazaarMarketComment.cpp    # 0x018B
    src/packets/game/PartyRecruitReply.cpp      # 0x018F
    src/packets/game/PartyRecruit.cpp           # 0x0192
    src/packets/game/StatusIcon.cpp             # 0x0194
    src/packets/game/MapFlag.cpp                # 0x0197
    src/packets/game/DemonCompendium.cpp        # 0x019B
    src/packets/game/ItemRepairMax.cpp          # 0x019F
    src/packets/game/AppearanceAlter.cpp        # 0x01A2
    src/packets/game/EntrustRequest.cpp         # 0x01AA
    src/packets/game/EntrustAccept.cpp          # 0x01AD
    src/packets/game/EntrustRewardUpdate.cpp    # 0x01AF
    src/packets/game/EntrustRewardFinish.cpp    # 0x01B2
    src/packets/game/EntrustRewardAccept.cpp    # 0x01B4
    src/packets/game/EntrustFinish.cpp          # 0x01B6
    src/packets/game/DemonCrystallizeItem.cpp   # 0x01B8
    src/packets/game/DemonCrystallize.cpp       # 0x01BB
    src/packets/game/EnchantItem.cpp            # 0x01BE
    src/packets/game/Enchant.cpp                # 0x01C1
    src/packets/game/DungeonRecords.cpp         # 0x01C4
    src/packets/game/ItemPromo.cpp              # 0x01C9
    src/packets/game/TriFusionJoin.cpp          # 0x01CD
    src/packets/game/TriFusionDemonUpdate.cpp   # 0x01D0
    src/packets/game/TriFusionRewardUpdate.cpp  # 0x01D3
    src/packets/game/TriFusionRewardAccept.cpp  # 0x01D6
    src/packets/game/TriFusionAccept.cpp        # 0x01D9
    src/packets/game/TriFusionLeave.cpp         # 0x01DE
    src/packets/game/ClanEmblemUpdate.cpp       # 0x01E1
    src/packets/game/DemonFamiliarity.cpp       # 0x01E6
    src/packets/game/PlasmaStart.cpp            # 0x01EB
    src/packets/game/PlasmaResult.cpp           # 0x01ED
    src/packets/game/PlasmaEnd.cpp              # 0x01EF
    src/packets/game/PlasmaItemData.cpp         # 0x01F1
    src/packets/game/PlasmaItem.cpp             # 0x01F3
    src/packets/game/TimeLimitSync.cpp          # 0x01F9
    src/packets/game/ItemDisassemble.cpp        # 0x01FC
    src/packets/game/SynthesizeRecipe.cpp       # 0x01FE
    src/packets/game/Synthesize.cpp             # 0x0200
    src/packets/game/EquipmentMod.cpp           # 0x0203
    src/packets/game/MaterialBox.cpp            # 0x0205
    src/packets/game/Analyze.cpp                # 0x0209
    src/packets/game/MaterialExtract.cpp        # 0x020C
    src/packets/game/MaterialInsert.cpp         # 0x020E
    src/packets/game/ItemExchange.cpp           # 0x0210
    src/packets/game/CompShopOpen.cpp           # 0x0212
    src/packets/game/CompShopList.cpp           # 0x0214
    src/packets/game/FusionGauge.cpp            # 0x0217
    src/packets/game/TitleList.cpp              # 0x021B
    src/packets/game/TitleActiveUpdate.cpp      # 0x021D
    src/packets/game/TitleBuild.cpp             # 0x0220
    src/packets/game/DemonQuestData.cpp         # 0x0224
    src/packets/game/DemonQuestAccept.cpp       # 0x0226
    src/packets/game/DemonQuestEnd.cpp          # 0x0229
    src/packets/game/DemonQuestCancel.cpp       # 0x022B
    src/packets/game/DemonQuestList.cpp         # 0x022D
    src/packets/game/DemonQuestActive.cpp       # 0x022F
    src/packets/game/DemonLock.cpp              # 0x0233
    src/packets/game/DemonReunion.cpp           # 0x0235
    src/packets/game/DemonQuestReject.cpp       # 0x023A
    src/packets/game/PvPConfirm.cpp             # 0x023E
    src/packets/game/PvPBaseCapture.cpp         # 0x0242
    src/packets/game/PvPBaseLeave.cpp           # 0x0247
    src/packets/game/PvPJoin.cpp                # 0x0249
    src/packets/game/PvPCancel.cpp              # 0x024B
    src/packets/game/PvPCharacterInfo.cpp       # 0x024D
    src/packets/game/AutoRecoveryUpdate.cpp     # 0x025A
    src/packets/game/ItemMix.cpp                # 0x025D
    src/packets/game/BikeBoostOn.cpp            # 0x0260
    src/packets/game/BikeBoostOff.cpp           # 0x0262
    src/packets/game/BikeDismount.cpp           # 0x0264
    src/packets/game/TeamForm.cpp               # 0x0267
    src/packets/game/TeamInvite.cpp             # 0x0269
    src/packets/game/TeamAnswer.cpp             # 0x026C
    src/packets/game/TeamKick.cpp               # 0x0270
    src/packets/game/TeamLeaderUpdate.cpp       # 0x0273
    src/packets/game/TeamLeave.cpp              # 0x0276
    src/packets/game/TeamChat.cpp               # 0x0279
    src/packets/game/TeamInfo.cpp               # 0x027B
    src/packets/game/TeamMemberList.cpp         # 0x027D
    src/packets/game/EquipmentSpiritFuse.cpp    # 0x0287
    src/packets/game/DemonQuestPending.cpp      # 0x028F
    src/packets/game/ItemDepoRemote.cpp         # 0x0296
    src/packets/game/DiasporaBaseCapture.cpp    # 0x02C4
    src/packets/game/DiasporaEnter.cpp          # 0x02C7
    src/packets/game/DemonDepoRemote.cpp        # 0x02EF
    src/packets/game/CommonSwitchUpdate.cpp     # 0x02F2
    src/packets/game/CommonSwitchInfo.cpp       # 0x02F4
    src/packets/game/DemonForce.cpp             # 0x02F6
    src/packets/game/DemonForceStack.cpp        # 0x02F8
    src/packets/game/CasinoCoinTotal.cpp        # 0x02FA
    src/packets/game/TriFusionSolo.cpp          # 0x0384
    src/packets/game/EquipmentSpiritDefuse.cpp  # 0x0386
    src/packets/game/DemonForceEnd.cpp          # 0x0389
    src/packets/game/UBSpectatePlayer.cpp       # 0x038F
    src/packets/game/UBProceed.cpp              # 0x0392
    src/packets/game/UBLeave.cpp                # 0x0394
    src/packets/game/UBLottoCancel.cpp          # 0x0398
    src/packets/game/UBLottoJoin.cpp            # 0x039A
    src/packets/game/SearchEntryInfo.cpp        # 0x03A3
    src/packets/game/ITimeData.cpp              # 0x03A5
    src/packets/game/ITimeTalk.cpp              # 0x03A9
    src/packets/game/CultureData.cpp            # 0x03AC
    src/packets/game/CultureMachineAccess.cpp   # 0x03AE
    src/packets/game/CultureStart.cpp           # 0x03B0
    src/packets/game/CultureItem.cpp            # 0x03B2
    src/packets/game/CultureEnd.cpp             # 0x03B6
    src/packets/game/EquipmentModEdit.cpp       # 0x03CB
    src/packets/game/PAttributeDeadline.cpp     # 0x03EA
    src/packets/game/MissionLeave.cpp           # 0x03ED
    src/packets/game/MitamaReunion.cpp          # 0x03F1
    src/packets/game/MitamaReset.cpp            # 0x03F3
    src/packets/game/DemonDepoList.cpp          # 0x03F5
    src/packets/game/DemonEquip.cpp             # 0x03FB
    src/packets/game/Barter.cpp                 # 0x03FE
    src/packets/game/PentalphaData.cpp          # 0x0400
    src/packets/game/QuestTitle.cpp             # 0x0406
    src/packets/game/ReportPlayer.cpp           # 0x0407
    src/packets/game/Blacklist.cpp              # 0x0408
    src/packets/game/BlacklistUpdate.cpp        # 0x040A
    src/packets/game/DestinyBoxData.cpp         # 0x040D
    src/packets/game/DestinyLotto.cpp           # 0x0410
    src/packets/game/DigitalizePoints.cpp       # 0x0414
    src/packets/game/DigitalizeAssist.cpp       # 0x0418
    src/packets/game/DigitalizeAssistLearn.cpp  # 0x041A
    src/packets/game/DigitalizeAssistRemove.cpp # 0x041C
    src/packets/game/VABox.cpp                  # 0x041E
    src/packets/game/VABoxAdd.cpp               # 0x0420
    src/packets/game/VABoxRemove.cpp            # 0x0422
    src/packets/game/VAChange.cpp               # 0x0424
    src/packets/game/VABoxMove.cpp              # 0x0427
    src/packets/game/ReunionPoints.cpp          # 0x0432
    src/packets/game/ReunionExtract.cpp         # 0x0434
    src/packets/game/ReunionInject.cpp          # 0x0436

    # Amala Network packets
    src/packets/amala/AccountDumpRequest.cpp    # 0x1000

    #Internal packets
    src/packets/internal/SetWorldInfo.cpp          # 0x1002
    src/packets/internal/SetOtherChannelInfo.cpp   # 0x1003
    src/packets/internal/AccountLogin.cpp          # 0x1004
    src/packets/internal/AccountLogout.cpp         # 0x1005
    src/packets/internal/Relay.cpp                 # 0x1006
    src/packets/internal/DataSync.cpp              # 0x1007
    src/packets/internal/CharacterLogin.cpp        # 0x1008
    src/packets/internal/FriendsUpdate.cpp         # 0x1009
    src/packets/internal/PartyUpdate.cpp           # 0x100A
    src/packets/internal/ClanUpdate.cpp            # 0x100B
    src/packets/internal/WebGame.cpp               # 0x100C
    src/packets/internal/TeamUpdate.cpp            # 0x100D
)

IF(SINGLE_SOURCE_PACKETS)
    COMBINE_PACKETS(${PROJECT_NAME}_PACKETS)
ENDIF(SINGLE_SOURCE_PACKETS)

COVERALLS_SOURCES(
    ${${PROJECT_NAME}_SRCS}
    ${${PROJECT_NAME}_PACKETS}
)

ADD_EXECUTABLE(${PROJECT_NAME} ${${PROJECT_NAME}_SRCS}
    ${${PROJECT_NAME}_HDRS} ${${PROJECT_NAME}_PACKETS}
    ${${PROJECT_NAME}_STRUCTS})

ADD_DEPENDENCIES(${PROJECT_NAME} asio)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER "Server")

TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/objgen
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT} config comp
    tinyxml2 civetweb-cxx civetweb)

IF(USE_COTIRE)
    cotire(${PROJECT_NAME})
ENDIF(USE_COTIRE)

UPX_WRAP(${PROJECT_NAME})

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${COMP_INSTALL_DIR} COMPONENT channel)

# Include the PDB file if on Windows
IF(WIN32)
    INSTALL(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION ${COMP_INSTALL_DIR} COMPONENT channel)
ENDIF(WIN32)

ENDIF(IMPORT_CHANNEL)
