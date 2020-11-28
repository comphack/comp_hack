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

PROJECT(libclient)

MESSAGE("** Configuring ${PROJECT_NAME} **")

SET(${PROJECT_NAME}_SRCS
    src/LogicWorker.cpp

    # Managers
    src/AmalaManager.cpp
    src/ConnectionManager.cpp
    src/LobbyManager.cpp
)

SET(${PROJECT_NAME}_HDRS
    src/ClientManager.h
    src/LogicWorker.h
    src/MessageClient.h

    # Managers
    src/AmalaManager.h
    src/ConnectionManager.h
    src/LobbyManager.h

    # Messages
    src/MessageClient.h
    src/MessageCharacterList.h
    src/MessageConnected.h
    src/MessageConnectionInfo.h
    src/MessageStartGame.h
)

ADD_LIBRARY(client STATIC ${${PROJECT_NAME}_SRCS}
    ${${PROJECT_NAME}_HDRS})

IF(BUILD_EXOTIC)
    SET_SOURCE_FILES_LANGUAGE(${${PROJECT_NAME}_SRCS})
ENDIF(BUILD_EXOTIC)

IF(NOT BUILD_EXOTIC)
    ADD_DEPENDENCIES(client asio)
ENDIF(NOT BUILD_EXOTIC)

SET_TARGET_PROPERTIES(client PROPERTIES FOLDER "Libraries")

TARGET_INCLUDE_DIRECTORIES(client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

TARGET_LINK_LIBRARIES(client ${CMAKE_THREAD_LIBS_INIT} packets comp)
