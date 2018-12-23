# This file is part of COMP_hack.
#
# Copyright (C) 2010-2018 COMP_hack Team <compomega@tutanota.com>
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

MESSAGE("** Configuring ${PROJECT_NAME} **")

SET(${PROJECT_NAME}_SRCS
    src/main.cpp
)

SET(${PROJECT_NAME}_HDRS
)

ADD_EXECUTABLE(${PROJECT_NAME} ${${PROJECT_NAME}_SRCS}
    ${${PROJECT_NAME}_HDRS})

ADD_DEPENDENCIES(${PROJECT_NAME} asio git-version)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER "Client")

TARGET_LINK_LIBRARIES(${PROJECT_NAME} tester)

UPX_WRAP(${PROJECT_NAME})

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${COMP_INSTALL_DIR} COMPONENT tools)
