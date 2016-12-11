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

PROJECT(comp_lobby)

MESSAGE("** Configuring ${PROJECT_NAME} **")

# Add a directory to put the objgen output into.
FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/objgen)

SET(${PROJECT_NAME}_SRCS
    src/LobbyServer.cpp
    src/LoginWebHandler.cpp
    src/ManagerConnection.cpp
    src/ManagerPacket.cpp
    src/ManagerPacketClient.cpp
    src/ManagerPacketInternal.cpp
    src/World.cpp
    src/main.cpp

    ${CMAKE_CURRENT_BINARY_DIR}/res/login/ResourceLogin.c
)

SET(${PROJECT_NAME}_HDRS
    src/LobbyServer.h
    src/LoginWebHandler.h
    src/ManagerConnection.h
    src/ManagerPacket.h
    src/ManagerPacketClient.h
    src/ManagerPacketInternal.h
    src/PacketParser.h
    src/World.h

    ${CMAKE_CURRENT_BINARY_DIR}/res/login/ResourceLogin.h
)

SET(${PROJECT_NAME}_SCHEMA
    schema/lobbyconfig.xml
)

OBJGEN_XML(${PROJECT_NAME}_STRUCTS
    # Main schema
    schema/master.xml

    # Included schemas
    ${${PROJECT_NAME}_SCHEMA}

    # Include paths
    schema

    # Output files
    LobbyConfig.h
    LobbyConfig.cpp
    PacketLogin.h
    PacketLogin.cpp
    PacketResponseCode.h
    PacketResponseCode.cpp
)

SOURCE_GROUP("Source Files\\Packets" src/packets/*)

SET(${PROJECT_NAME}_PACKETS
    src/packets/Login.cpp               # 0x0003
    src/packets/Auth.cpp                # 0x0005
    src/packets/StartGame.cpp           # 0x0007
    src/packets/CharacterList.cpp       # 0x0009
    src/packets/WorldList.cpp           # 0x000B
    src/packets/CreateCharacter.cpp     # 0x000D
    src/packets/DeleteCharacter.cpp     # 0x000F
    src/packets/QueryPurchaseTicket.cpp # 0x0011
    src/packets/PurchaseTicket.cpp      # 0x0013
    
    #Internal packets
    src/packets/SetWorldDescription.cpp      # 0x1002
    src/packets/SetChannelDescription.cpp    # 0x1003
)

ADD_CUSTOM_COMMAND(
    OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/res/login/ResourceLogin.c
        ${CMAKE_CURRENT_BINARY_DIR}/res/login/ResourceLogin.h
    COMMAND ${CMAKE_COMMAND} -E make_directory
        ${CMAKE_CURRENT_BINARY_DIR}/res/login
    COMMAND ${TTVFS_GEN_PATH} ResourceLogin ResourceLoginSize
        ${CMAKE_CURRENT_SOURCE_DIR}/res/login
        ${CMAKE_CURRENT_BINARY_DIR}/res/login/ResourceLogin.c
        ${CMAKE_CURRENT_BINARY_DIR}/res/login/ResourceLogin.h
    DEPENDS ttvfs-ex
    COMMENT "Generating resource file for the login screen"
)

ADD_EXECUTABLE(${PROJECT_NAME} ${${PROJECT_NAME}_SRCS}
    ${${PROJECT_NAME}_HDRS} ${${PROJECT_NAME}_PACKETS}
    ${${PROJECT_NAME}_STRUCTS})

ADD_DEPENDENCIES(${PROJECT_NAME} asio)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER "Server")

TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/objgen
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/res/login
)

TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT} comp tinyxml2
    civetweb-cxx civetweb)

UPX_WRAP(${PROJECT_NAME})

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION bin)
