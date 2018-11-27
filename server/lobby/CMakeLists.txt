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

# Add a target to generate API documentation.
FILE(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/api/server/lobby)
GENERATE_DOCS(doc/Doxyfile.in)

# Add a directory to put the objgen output into.
FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/objgen)

SET(${PROJECT_NAME}_SRCS
    ${CMAKE_SOURCE_DIR}/libcomp/src/WindowsServiceMain.cpp

    src/AccountManager.cpp
    src/ApiHandler.cpp
    src/ClientState.cpp
    src/ImportHandler.cpp
    src/LobbyClientConnection.cpp
    src/LobbyServer.cpp
    src/LoginHandlerThread.cpp
    src/LobbySyncManager.cpp
    src/LoginWebHandler.cpp
    src/ManagerClientPacket.cpp
    src/ManagerConnection.cpp
    src/World.cpp
    src/main.cpp
)

SET(${PROJECT_NAME}_RES_SRCS
    ${CMAKE_CURRENT_BINARY_DIR}/res/login/ResourceLogin.c
)

SET(${PROJECT_NAME}_HDRS
    src/AccountManager.h
    src/ApiHandler.h
    src/ClientState.h
    src/LobbyClientConnection.h
    src/LobbyServer.h
    src/LoginHandlerThread.h
    src/LobbySyncManager.h
    src/LoginWebHandler.h
    src/ManagerClientPacket.h
    src/ManagerConnection.h
    src/World.h

    ${CMAKE_CURRENT_BINARY_DIR}/res/login/ResourceLogin.h
)

SET(${PROJECT_NAME}_SCHEMA
    schema/clientstate.xml
    schema/lobbyconfig.xml
    schema/loginscript.xml
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
    ClientStateObject.h
    ClientStateObject.cpp
    LobbyConfig.h
    LobbyConfig.cpp
    LoginScriptRequest.h
    LoginScriptRequest.cpp
    LoginScriptReply.h
    LoginScriptReply.cpp
)

SOURCE_GROUP("Source Files\\Packets\\Game" src/packets/game/*)
SOURCE_GROUP("Source Files\\Packets\\Internal" src/packets/internal/*)

SET(${PROJECT_NAME}_PACKETS
    src/packets/game/Login.cpp               # 0x0003
    src/packets/game/Auth.cpp                # 0x0005
    src/packets/game/StartGame.cpp           # 0x0007
    src/packets/game/CharacterList.cpp       # 0x0009
    src/packets/game/WorldList.cpp           # 0x000B
    src/packets/game/CreateCharacter.cpp     # 0x000D
    src/packets/game/DeleteCharacter.cpp     # 0x000F
    src/packets/game/QueryPurchaseTicket.cpp # 0x0011
    src/packets/game/PurchaseTicket.cpp      # 0x0013

    #Internal packets
    src/packets/internal/SetWorldInfo.cpp      # 0x1002
    src/packets/internal/SetChannelInfo.cpp    # 0x1003
    src/packets/internal/AccountLogin.cpp      # 0x1004
    src/packets/internal/AccountLogout.cpp     # 0x1005
    src/packets/internal/DataSync.cpp          # 0x1007
    src/packets/internal/WebGame.cpp           # 0x100C
)

IF(SINGLE_SOURCE_PACKETS)
    COMBINE_PACKETS(${PROJECT_NAME}_PACKETS)
ENDIF(SINGLE_SOURCE_PACKETS)

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

COVERALLS_SOURCES(
    ${${PROJECT_NAME}_SRCS}
    ${${PROJECT_NAME}_PACKETS}
)

ADD_EXECUTABLE(${PROJECT_NAME} ${${PROJECT_NAME}_SRCS}
    ${${PROJECT_NAME}_HDRS} ${${PROJECT_NAME}_PACKETS}
    ${${PROJECT_NAME}_STRUCTS} ${${PROJECT_NAME}_RES_SRCS})

ADD_DEPENDENCIES(${PROJECT_NAME} asio)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER "Server")

TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/objgen
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/res/login
)

TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT} comp tinyxml2
    civetweb-cxx civetweb jsonbox)

IF(USE_COTIRE)
    cotire(${PROJECT_NAME})
ENDIF(USE_COTIRE)

UPX_WRAP(${PROJECT_NAME})

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${COMP_INSTALL_DIR})

# Include the PDB file if on Windows
IF(WIN32)
    INSTALL(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION ${COMP_INSTALL_DIR})
ENDIF(WIN32)

RSPEC_TESTS(
    LobbyAPI
)
