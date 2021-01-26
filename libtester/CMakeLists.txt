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

PROJECT(libtester)

MESSAGE("** Configuring ${PROJECT_NAME} **")

SET(${PROJECT_NAME}_SRCS
    src/ChannelClient.cpp
    src/HttpConnection.cpp
    src/LobbyClient.cpp
    src/Login.cpp
    src/ServerTest.cpp
    src/TestClient.cpp

    src/ChannelClient_HandleCharacterData.cpp
    src/ChannelClient_HandleDemonBoxData.cpp
    src/ChannelClient_HandleZoneChange.cpp

    src/ChannelClient_HandleAmalaServerVersion.cpp
    src/ChannelClient_HandleAmalaAccountDumpHeader.cpp
    src/ChannelClient_HandleAmalaAccountDumpPart.cpp
)

SET(${PROJECT_NAME}_HDRS
    src/ChannelClient.h
    src/HttpConnection.h
    src/LobbyClient.h
    src/Login.h
    src/ServerTest.h
    src/TestClient.h
)

ADD_LIBRARY(tester STATIC ${${PROJECT_NAME}_SRCS}
    ${${PROJECT_NAME}_HDRS})

ADD_DEPENDENCIES(tester asio)

SET_TARGET_PROPERTIES(tester PROPERTIES FOLDER "Libraries")

TARGET_INCLUDE_DIRECTORIES(tester PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

TARGET_LINK_LIBRARIES(tester ${CMAKE_THREAD_LIBS_INIT} hack comp tinyxml2 gtest)

# Commenting out the Lobby test until it does something useful
# List of unit tests to add to CTest.
SET(${PROJECT_NAME}_TEST_SRCS
    Lobby
    ChannelLogin
)

IF(NOT BSD)
    ADD_CUSTOM_TARGET(client-test DEPENDS comp_lobby comp_channel comp_world)

    # Add the unit tests.
    CREATE_GTESTS(TARGET client-test LIBS comp tester
        SRCS ${${PROJECT_NAME}_TEST_SRCS})
ENDIF(NOT BSD)
