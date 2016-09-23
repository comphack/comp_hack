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

INCLUDE_DIRECTORIES(${LIBCOMP_INCLUDES})
INCLUDE_DIRECTORIES(${ASIO_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${TTVFS_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${OPENSSL_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${CIVETWEB_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${TINYXML2_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/res/login)

SET(${PROJECT_NAME}_SRCS
    src/ChannelServer.cpp
    src/ChannelHandler.cpp
    src/main.cpp

    ${CMAKE_CURRENT_BINARY_DIR}/res/login/ResourceLogin.c
)

SET(${PROJECT_NAME}_HDRS
    src/ChannelServer.h
    src/ChannelHandler.h

    ${CMAKE_CURRENT_BINARY_DIR}/res/login/ResourceLogin.h
)

SET(${PROJECT_NAME}_PACKETS
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
    ${${PROJECT_NAME}_HDRS} ${${PROJECT_NAME}_PACKETS})

ADD_DEPENDENCIES(${PROJECT_NAME} asio)

TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT} comp tinyxml2
    civetweb-cxx civetweb)

UPX_WRAP(${PROJECT_NAME})

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION bin)
