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

PROJECT(comp_translator)

MESSAGE("** Configuring ${PROJECT_NAME} **")

# ADD_EXE_LINKER_FLAGS(AUTO -static)

SET(${PROJECT_NAME}_SRCS
    src/translator.cpp
)

SET(${PROJECT_NAME}_HDRS
)

ADD_EXECUTABLE(${PROJECT_NAME} ${${PROJECT_NAME}_SRCS} ${${PROJECT_NAME}_HDRS})

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER "Tools")

TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PUBLIC
    ../bdpatch/src
)

IF(EXISTS "${CMAKE_SOURCE_DIR}/dream" AND BUILD_DREAM)
    TARGET_COMPILE_DEFINITIONS(${PROJECT_NAME} PUBLIC
        "DREAM_OBJGEN_INCLUDE_A=${DREAM_OBJGEN_INCLUDE_A}"
        "DREAM_OBJGEN_INCLUDE_B=${DREAM_OBJGEN_INCLUDE_B}"
    )

    TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${DREAM_OBJGEN_LIB})
ENDIF(EXISTS "${CMAKE_SOURCE_DIR}/dream" AND BUILD_DREAM)

TARGET_LINK_LIBRARIES(${PROJECT_NAME} comp)

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${COMP_INSTALL_DIR} COMPONENT tools)
