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

CMAKE_MINIMUM_REQUIRED(VERSION 3.5)

# This is the command line version.
ADD_SUBDIRECTORY(src)

PROJECT(comp_logger)

SET(CMAKE_AUTOMOC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Network REQUIRED)
FIND_PACKAGE(Qt5Xml REQUIRED)

MESSAGE("** Configuring ${PROJECT_NAME} **")

# Add a target to generate API documentation.
FILE(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/api/tools/logger)
GENERATE_DOCS(doc/Doxyfile.in)

INCLUDE_DIRECTORIES(${ZLIB_INCLUDES})
INCLUDE_DIRECTORIES(${LIBCOMP_INCLUDES})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

SET(${PROJECT_NAME}_SRCS
    src/ChannelConnection.cpp
    src/Close.cpp
    src/LobbyConnection.cpp
    src/LoggerServer.cpp
    src/MainWindow.cpp
    src/Settings.cpp
    src/WebAuthConnection.cpp
    src/main.cpp
)

SET(${PROJECT_NAME}_HDRS
    src/ChannelConnection.h
    src/Close.h
    src/LobbyConnection.h
    src/LoggerServer.h
    src/MainWindow.h
    src/Settings.h
    src/WebAuthConnection.h
)

SET(${PROJECT_NAME}_RCC
    res/logger.qrc
)
qt5_add_resources(${PROJECT_NAME}_RCC_SRCS ${${PROJECT_NAME}_RCC})

SET(${PROJECT_NAME}_UIS
    ui/About.ui
    ui/Close.ui
    ui/MainWindow.ui
    ui/Settings.ui
)
qt5_wrap_ui(${PROJECT_NAME}_UI_SRCS ${${PROJECT_NAME}_UIS})

RES_WRAP(${PROJECT_NAME}_SRCS res/logger.rc)

ADD_EXECUTABLE(${PROJECT_NAME} WIN32 ${${PROJECT_NAME}_SRCS}
    ${${PROJECT_NAME}_HDRS} ${${PROJECT_NAME}_RCC_SRCS}
    ${${PROJECT_NAME}_UI_SRCS})

IF(MINGW)
    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS -mwindows)
ENDIF(MINGW)

UPX_WRAP(${PROJECT_NAME})

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER "Tools")

TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${OPENSSL_LIBRARIES} comp Qt5::Widgets
	Qt5::Network Qt5::Xml zlib)

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION bin)

IF(NOT WIN32)
    INSTALL(FILES res/${PROJECT_NAME}.desktop DESTINATION share/applications)
ENDIF(NOT WIN32)
