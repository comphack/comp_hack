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

PROJECT(libclient)

MESSAGE("** Configuring ${PROJECT_NAME} **")

SET(${PROJECT_NAME}_SRCS
    src/LogicWorker.cpp

    # Managers
    src/ConnectionManager.cpp
    src/LobbyManager.cpp
)

SET(${PROJECT_NAME}_HDRS
    src/ClientManager.h
    src/LogicWorker.h

    # Managers
    src/ConnectionManager.h
    src/LobbyManager.h

    # Messages
    src/MessageConnected.h
    src/MessageConnectionInfo.h
)

# Don't try to build the headers.
IF(BUILD_EXOTIC)
    SET(${PROJECT_NAME}_HDRS "")
ENDIF(BUILD_EXOTIC)

ADD_LIBRARY(client STATIC ${${PROJECT_NAME}_SRCS}
    ${${PROJECT_NAME}_HDRS})

IF(BUILD_EXOTIC)
    SET_SOURCE_FILES_PROPERTIES(${${PROJECT_NAME}_SRCS} ${${PROJECT_NAME}_HDRS}
        PROPERTIES LANGUAGE ${EXOTIC_LANGUAGE})
ENDIF(BUILD_EXOTIC)

IF(NOT BUILD_EXOTIC)
    ADD_DEPENDENCIES(client asio)
ENDIF(NOT BUILD_EXOTIC)

SET_TARGET_PROPERTIES(client PROPERTIES FOLDER "Libraries")

TARGET_INCLUDE_DIRECTORIES(client PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

TARGET_LINK_LIBRARIES(client ${CMAKE_THREAD_LIBS_INIT} packets comp)
