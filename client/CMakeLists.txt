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

PROJECT(comp_client)

SET(CMAKE_AUTOMOC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

# Policy for automoc of generated files (UI headers).
IF("3.10.0" VERSION_LESS ${CMAKE_VERSION})
    CMAKE_POLICY(SET CMP0071 NEW)
ENDIF("3.10.0" VERSION_LESS ${CMAKE_VERSION})

FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Network REQUIRED)

MESSAGE("** Configuring ${PROJECT_NAME} **")

SET(${PROJECT_NAME}_SRCS
    src/main.cpp

    src/ChannelScene.cpp
    src/GameWorker.cpp
    src/LobbyScene.cpp
    src/LoginDialog.cpp
)

SET(${PROJECT_NAME}_HDRS
    src/ChannelScene.h
    src/GameWorker.h
    src/LobbyScene.h
    src/LoginDialog.h
)

SET(${PROJECT_NAME}_RCC
    # res/logger.qrc
)
qt5_add_resources(${PROJECT_NAME}_RCC_SRCS ${${PROJECT_NAME}_RCC})

SET(${PROJECT_NAME}_UIS
    ui/ChannelScene.ui
    ui/LobbyScene.ui
    ui/LoginDialog.ui
)
qt5_wrap_ui(${PROJECT_NAME}_UI_SRCS ${${PROJECT_NAME}_UIS})

# RES_WRAP(${PROJECT_NAME}_SRCS res/logger.rc)

ADD_EXECUTABLE(${PROJECT_NAME} ${${PROJECT_NAME}_SRCS}
    ${${PROJECT_NAME}_HDRS} ${${PROJECT_NAME}_RCC_SRCS}
    ${${PROJECT_NAME}_UI_SRCS})

# IF(MINGW)
#     SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS -mwindows)
# ENDIF(MINGW)

UPX_WRAP(${PROJECT_NAME})

ADD_DEPENDENCIES(${PROJECT_NAME} asio git-version)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER "Client")

TARGET_LINK_LIBRARIES(${PROJECT_NAME} client Qt5::Widgets Qt5::Network)

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${COMP_INSTALL_DIR} COMPONENT tools)

# IF(NOT WIN32)
#     INSTALL(FILES res/${PROJECT_NAME}.desktop DESTINATION share/applications)
# ENDIF(NOT WIN32)
