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
    src/ChannelClientConnection.cpp
    src/ChannelServer.cpp
    src/CharacterManager.cpp
    src/CharacterState.cpp
    src/ClientState.cpp
    src/ManagerConnection.cpp
    src/ChatManager.cpp
    src/main.cpp
    src/packets/game/Chat.cpp
)

SET(${PROJECT_NAME}_HDRS
    src/AccountManager.h
    src/ChannelClientConnection.h
    src/ChannelServer.h
    src/CharacterManager.h
    src/CharacterState.h
    src/ClientState.h
    src/ManagerConnection.h
    src/ChatManager.h
    src/Packets.h
)

SET(${PROJECT_NAME}_SCHEMA
    schema/channelconfig.xml
    schema/characterstate.xml
    schema/clientstate.xml
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
    ChannelConfig.h
    ChannelConfig.cpp
    CharacterStateObject.h
    CharacterStateObject.cpp
    ClientStateObject.h
    ClientStateObject.cpp
)

SOURCE_GROUP("Source Files\\Packets\\Game" src/packets/game/*)
SOURCE_GROUP("Source Files\\Packets\\Internal" src/packets/internal/*)

SET(${PROJECT_NAME}_PACKETS
    src/packets/game/Login.cpp               # 0x0000
    src/packets/game/Auth.cpp                # 0x0002
    src/packets/game/SendData.cpp            # 0x0004
    src/packets/game/Logout.cpp              # 0x0005
<<<<<<< HEAD
<<<<<<< HEAD
    src/packets/game/Chat.cpp                # 0x0026
=======
    src/packets/game/Chat.cpp             # 0x0026
>>>>>>> 	modified:   libcomp/schema/account.xml
=======
    src/packets/game/Chat.cpp                # 0x0026
>>>>>>> 	modified:   server/channel/CMakeLists.txt
    src/packets/game/KeepAlive.cpp           # 0x0056
    src/packets/game/State.cpp               # 0x005A
    src/packets/game/Sync.cpp                # 0x00F3

    #Internal packets
    src/packets/internal/SetWorldInfo.cpp    # 0x1002
    src/packets/internal/AccountLogin.cpp    # 0x1004
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
