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

PROJECT(comp_channel)

MESSAGE("** Configuring ${PROJECT_NAME} **")

# Add a target to generate API documentation.
FILE(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/api/server/channel)
GENERATE_DOCS(doc/Doxyfile.in)

# Add a directory to put the objgen output into.
FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/objgen)

# Custom command to put the Git version in the @version command.
ADD_CUSTOM_TARGET(git-version ${CMAKE_COMMAND}
    -D "SRC=${CMAKE_CURRENT_SOURCE_DIR}/src/Git.h.in"
    -D "DST=${CMAKE_CURRENT_BINARY_DIR}/Git.h"
    -P ${CMAKE_SOURCE_DIR}/cmake/git-version.cmake
)

SET(${PROJECT_NAME}_SRCS
    src/AccountManager.cpp
    src/ActionManager.cpp
    src/ActiveEntityState.cpp
    src/ChannelClientConnection.cpp
    src/ChannelServer.cpp
    src/CharacterManager.cpp
    src/ChatManager.cpp
    src/ClientState.cpp
    src/EnemyState.cpp
    src/EntityState.cpp
    src/EventManager.cpp
    src/ManagerClientPacket.cpp
    src/ManagerConnection.cpp
    src/ManagerSystem.cpp
    src/SkillManager.cpp
    src/Zone.cpp
    src/ZoneManager.cpp
    src/main.cpp
)

SET(${PROJECT_NAME}_HDRS
    src/AccountManager.h
    src/ActionManager.h
    src/ActiveEntityState.h
    src/ChannelClientConnection.h
    src/ChannelServer.h
    src/CharacterManager.h
    src/ChatManager.h
    src/ClientState.h
    src/EnemyState.h
    src/EntityState.h
    src/EventManager.h
    src/ManagerClientPacket.h
    src/ManagerConnection.h
    src/ManagerSystem.h
    src/Packets.h
    src/SkillManager.h
    src/Zone.h
    src/ZoneManager.h
)

SET(${PROJECT_NAME}_SCHEMA
    schema/channelconfig.xml
    schema/clientstate.xml
    schema/entitystate.xml
)

SOURCE_GROUP("objgen" ${CMAKE_CURRENT_BINARY_DIR}/objgen/*)

OBJGEN_XML(${PROJECT_NAME}_STRUCTS
    # Main schema
    schema/master.xml

    # Included schemas
    ${${PROJECT_NAME}_SCHEMA}

    # Include paths
    schema
    ../../libcomp/schema

    # Output files
    ActivatedAbility.h
    ActivatedAbility.cpp
    ActiveEntityStateObject.h
    ActiveEntityStateObject.cpp
    ChannelConfig.h
    ChannelConfig.cpp
    ClientStateObject.h
    ClientStateObject.cpp
    EntityStateObject.h
    EntityStateObject.cpp
    EventInstance.h
    EventInstance.cpp
    EventState.h
    EventState.cpp
    TradeSession.h
    TradeSession.cpp
)

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
    src/packets/game/Chat.cpp                   # 0x0026
    src/packets/game/ActivateSkill.cpp          # 0x0030
    src/packets/game/ExecuteSkill.cpp           # 0x0031
    src/packets/game/CancelSkill.cpp            # 0x0032
    src/packets/game/AllocateSkillPoint.cpp     # 0x0049
    src/packets/game/ToggleExpertise.cpp        # 0x004F
    src/packets/game/LearnSkill.cpp             # 0x0051
    src/packets/game/UpdateDemonSkill.cpp       # 0x0054
    src/packets/game/KeepAlive.cpp              # 0x0056
    src/packets/game/FixObjectPosition.cpp      # 0x0058
    src/packets/game/State.cpp                  # 0x005A
    src/packets/game/PartnerDemonData.cpp       # 0x005B
    src/packets/game/DemonBox.cpp               # 0x005C
    src/packets/game/DemonBoxData.cpp           # 0x005E
    src/packets/game/ChannelList.cpp            # 0x0063
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
    src/packets/game/CashBalance.cpp            # 0x0090
    src/packets/game/ShopData.cpp               # 0x0092
    src/packets/game/ShopBuy.cpp                # 0x0094
    src/packets/game/ShopSell.cpp               # 0x0096
    src/packets/game/DemonBoxMove.cpp           # 0x009A
    src/packets/game/DismissDemon.cpp           # 0x009B
    src/packets/game/HotbarData.cpp             # 0x00A2
    src/packets/game/HotbarSave.cpp             # 0x00A4
    src/packets/game/EventResponse.cpp          # 0x00B7
    src/packets/game/ValuableList.cpp           # 0x00B8
    src/packets/game/ObjectInteraction.cpp      # 0x00BA
    src/packets/game/FriendInfo.cpp             # 0x00BD
    src/packets/game/FriendRequest              # 0x00C0
    src/packets/game/FriendAddRemove            # 0x00C3
    src/packets/game/FriendData                 # 0x00C6
    src/packets/game/PartyInvite                # 0x00D2
    src/packets/game/PartyJoin                  # 0x00D5
    src/packets/game/PartyCancel                # 0x00D8
    src/packets/game/PartyLeave                 # 0x00DA
    src/packets/game/PartyDisband               # 0x00DD
    src/packets/game/PartyLeaderUpdate          # 0x00E0
    src/packets/game/PartyDropRule              # 0x00E3
    src/packets/game/PartyKick                  # 0x00EB
    src/packets/game/Sync.cpp                   # 0x00F3
    src/packets/game/Rotate.cpp                 # 0x00F8
    src/packets/game/UnionFlag.cpp              # 0x0100
    src/packets/game/ItemDepoList.cpp           # 0x0102
    src/packets/game/DepoRent.cpp               # 0x0104
    src/packets/game/QuestActiveList.cpp        # 0x010B
    src/packets/game/QuestCompletedList.cpp     # 0x010C
    src/packets/game/ClanInfo.cpp               # 0x0150
    src/packets/game/MapFlag.cpp                # 0x0197
    src/packets/game/DemonCompendium.cpp        # 0x019B
    src/packets/game/DungeonRecords.cpp         # 0x01C4
    src/packets/game/MaterialBox.cpp            # 0x0205
    src/packets/game/FusionGauge.cpp            # 0x0217
    src/packets/game/TitleList.cpp              # 0x021B
    src/packets/game/PartnerDemonQuestList.cpp  # 0x022D
    src/packets/game/LockDemon.cpp              # 0x0233
    src/packets/game/PvPCharacterInfo.cpp       # 0x024D
    src/packets/game/TeamInfo.cpp               # 0x027B
    src/packets/game/PartnerDemonQuestTemp.cpp  # 0x028F
    src/packets/game/ItemDepoRemote.cpp         # 0x0296
    src/packets/game/DemonDepoRemote.cpp        # 0x02EF
    src/packets/game/CommonSwitchInfo.cpp       # 0x02F4
    src/packets/game/CasinoCoinTotal.cpp        # 0x02FA
    src/packets/game/SearchEntryInfo.cpp        # 0x03A3
    src/packets/game/HouraiData.cpp             # 0x03A5
    src/packets/game/CultureData.cpp            # 0x03AC
    src/packets/game/DemonDepoList.cpp          # 0x03F5
    src/packets/game/Blacklist.cpp              # 0x0408
    src/packets/game/DigitalizePoints.cpp       # 0x0414
    src/packets/game/DigitalizeAssist.cpp       # 0x0418

    #Internal packets
    src/packets/internal/SetWorldInfo.cpp          # 0x1002
    src/packets/internal/SetOtherChannelInfo.cpp   # 0x1003
    src/packets/internal/AccountLogin.cpp          # 0x1004
    src/packets/internal/CharacterLogin.cpp        # 0x1007
    src/packets/internal/FriendsUpdate.cpp         # 0x1008
    src/packets/internal/PartyUpdate.cpp           # 0x1009
)

ADD_EXECUTABLE(${PROJECT_NAME} ${${PROJECT_NAME}_SRCS}
    ${${PROJECT_NAME}_HDRS} ${${PROJECT_NAME}_PACKETS}
    ${${PROJECT_NAME}_STRUCTS})

ADD_DEPENDENCIES(${PROJECT_NAME} asio git-version)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER "Server")

TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/objgen
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT} comp tinyxml2
    civetweb-cxx civetweb)

UPX_WRAP(${PROJECT_NAME})

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION bin)
