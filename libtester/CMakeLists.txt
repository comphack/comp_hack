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

PROJECT(libtester)

MESSAGE("** Configuring ${PROJECT_NAME} **")

SET(${PROJECT_NAME}_SRCS
    src/LobbyClient.cpp
)

SET(${PROJECT_NAME}_HDRS
    src/LobbyClient.h
)

ADD_LIBRARY(tester STATIC ${${PROJECT_NAME}_SRCS}
    ${${PROJECT_NAME}_HDRS})

ADD_DEPENDENCIES(tester asio)

SET_TARGET_PROPERTIES(tester PROPERTIES FOLDER "Libraries")

TARGET_INCLUDE_DIRECTORIES(tester PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

TARGET_LINK_LIBRARIES(tester ${CMAKE_THREAD_LIBS_INIT} comp tinyxml2)

# List of unit tests to add to CTest.
SET(${PROJECT_NAME}_TEST_SRCS
    Lobby
)

IF(NOT BSD)
    # Add the unit tests.
    CREATE_GTESTS(LIBS comp tester SRCS ${${PROJECT_NAME}_TEST_SRCS})
ENDIF(NOT BSD)
