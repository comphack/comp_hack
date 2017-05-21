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

SET(${PROJECT_NAME}_SRCS
    src/AccountManager.cpp
    src/ActionManager.cpp
    src/ActiveEntityState.cpp
    src/ChannelClientConnection.cpp
    src/ChannelServer.cpp
    src/CharacterManager.cpp
    src/ChatManager.cpp
    src/ClientState.cpp
    src/EntityState.cpp
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
    src/EntityState.h
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
    src/packets/game/COMPList.cpp               # 0x005C
    src/packets/game/COMPDemonData.cpp          # 0x005E
    src/packets/game/ChannelList.cpp            # 0x0063
    src/packets/game/StopMovement.cpp           # 0x006F
    src/packets/game/SpotTriggered.cpp          # 0x0071
    src/packets/game/WorldTime.cpp              # 0x0072
    src/packets/game/ItemBox.cpp                # 0x0074
    src/packets/game/ItemMove.cpp               # 0x0076
    src/packets/game/ItemDrop.cpp               # 0x0077
    src/packets/game/ItemStack.cpp              # 0x0078
    src/packets/game/EquipmentList.cpp          # 0x007B
    src/packets/game/COMPSlotUpdate.cpp         # 0x009A
    src/packets/game/DismissDemon.cpp           # 0x009B
    src/packets/game/HotbarData.cpp             # 0x00A2
    src/packets/game/HotbarSave.cpp             # 0x00A4
    src/packets/game/ValuableList.cpp           # 0x00B8
    src/packets/game/ObjectInteraction.cpp      # 0x00BA
    src/packets/game/FriendInfo.cpp             # 0x00BD
    src/packets/game/Sync.cpp                   # 0x00F3
    src/packets/game/Rotate.cpp                 # 0x00F8
    src/packets/game/UnionFlag.cpp              # 0x0100
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
    src/packets/game/CommonSwitchInfo.cpp       # 0x02F4
    src/packets/game/CasinoCoinTotal.cpp        # 0x02FA
    src/packets/game/SearchEntryInfo.cpp        # 0x03A3
    src/packets/game/HouraiData.cpp             # 0x03A5
    src/packets/game/CultureData.cpp            # 0x03AC
    src/packets/game/Blacklist.cpp              # 0x0408
    src/packets/game/DigitalizePoints.cpp       # 0x0414
    src/packets/game/DigitalizeAssist.cpp       # 0x0418

    #Internal packets
    src/packets/internal/SetWorldInfo.cpp          # 0x1002
    src/packets/internal/SetOtherChannelInfo.cpp   # 0x1003
    src/packets/internal/AccountLogin.cpp          # 0x1004
)

ADD_EXECUTABLE(${PROJECT_NAME} ${${PROJECT_NAME}_SRCS}
    ${${PROJECT_NAME}_HDRS} ${${PROJECT_NAME}_PACKETS}
    ${${PROJECT_NAME}_STRUCTS})

ADD_DEPENDENCIES(${PROJECT_NAME} asio)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER "Server")

TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/objgen
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT} comp tinyxml2
    civetweb-cxx civetweb)

UPX_WRAP(${PROJECT_NAME})

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION bin)
