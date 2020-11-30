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

PROJECT(config)

MESSAGE("** Configuring ${PROJECT_NAME} **")

# Custom command to put the Git version in the @version command.
ADD_CUSTOM_TARGET(git-version COMMAND ${CMAKE_COMMAND}
    -D "SRC=${CMAKE_CURRENT_SOURCE_DIR}/src/Git.h.in"
    -D "DST=${CMAKE_CURRENT_BINARY_DIR}/Git.h"
    -P "${CMAKE_SOURCE_DIR}/cmake/git-version.cmake"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/Git.h.in"
        "${CMAKE_SOURCE_DIR}/cmake/git-version.cmake"
    BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/Git.h"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating Git.h"
    VERBATIM
)

SET(${PROJECT_NAME}_HDRS
    "${CMAKE_CURRENT_BINARY_DIR}/Git.h"

    src/ConfigLogVersion.cpp
)

ADD_LIBRARY(${PROJECT_NAME} ${${PROJECT_NAME}_SRCS} ${${PROJECT_NAME}_HDRS})

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER "Libraries")

TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
)

ADD_DEPENDENCIES(${PROJECT_NAME} git-version)

TARGET_LINK_LIBRARIES(${PROJECT_NAME} comp)
