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

PROJECT(libpackets)

MESSAGE("** Configuring ${PROJECT_NAME} **")

# Add a directory to put the objgen output into.
FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/objgen)

SET(${PROJECT_NAME}_SCHEMA
    schema/packet_login.xml
    schema/packet_login_reply.xml
    schema/packet_auth.xml
    schema/packet_auth_reply.xml
    schema/packet_worldlist.xml
    schema/packet_characterlist.xml
    schema/packet_req_start_game.xml
    schema/packet_start_game.xml
    schema/packet_account_dump.xml
)

SOURCE_GROUP("objgen" ${CMAKE_CURRENT_BINARY_DIR}/objgen/*)

OBJGEN_XML(${PROJECT_NAME}_STRUCTS
    # Main schema
    schema/libpackets-master.xml

    # Included schemas
    ${${PROJECT_NAME}_SCHEMA}

    # Include paths
    schema

    # Output files
    PacketLobbyLogin.cpp
    PacketLobbyLogin.h
    PacketLobbyLoginReply.cpp
    PacketLobbyLoginReply.h
    PacketLobbyAuth.cpp
    PacketLobbyAuth.h
    PacketLobbyAuthReply.cpp
    PacketLobbyAuthReply.h
    PacketLobbyWorldList.cpp
    PacketLobbyWorldList.h
    PacketLobbyWorldListEntry.cpp
    PacketLobbyWorldListEntry.h
    PacketLobbyWorldListChannelEntry.cpp
    PacketLobbyWorldListChannelEntry.h
    PacketLobbyCharacterList.h
    PacketLobbyCharacterList.cpp
    PacketLobbyCharacterListEntry.h
    PacketLobbyCharacterListEntry.cpp
    PacketLobbyCharacterListEquippedVAEntry.h
    PacketLobbyCharacterListEquippedVAEntry.cpp
    PacketLobbyRequestStartGame.h
    PacketLobbyRequestStartGame.cpp
    PacketLobbyStartGame.h
    PacketLobbyStartGame.cpp
    PacketChannelLogin.cpp
    PacketChannelLogin.h
    PacketChannelLoginReply.cpp
    PacketChannelLoginReply.h
    PacketChannelAuth.cpp
    PacketChannelAuth.h
    PacketChannelAuthReply.cpp
    PacketChannelAuthReply.h
    PacketChannelAmalaAccountDumpHeader.cpp
    PacketChannelAmalaAccountDumpHeader.h
    PacketChannelAmalaAccountDumpPart.cpp
    PacketChannelAmalaAccountDumpPart.h
)

ADD_LIBRARY(packets STATIC ${${PROJECT_NAME}_STRUCTS})

IF(BUILD_EXOTIC)
    SET_SOURCE_FILES_LANGUAGE(${${PROJECT_NAME}_STRUCTS})
ENDIF(BUILD_EXOTIC)

SET_TARGET_PROPERTIES(packets PROPERTIES FOLDER "Libraries")

TARGET_INCLUDE_DIRECTORIES(packets PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}/objgen
)

TARGET_LINK_LIBRARIES(packets comp)
