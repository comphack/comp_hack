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

PROJECT(comp_nifcrypt)

MESSAGE("** Configuring ${PROJECT_NAME} **")

SET(${PROJECT_NAME}_SRCS
    src/nifcrypt.cpp
)

SET(${PROJECT_NAME}_HDRS
)

ADD_EXECUTABLE(${PROJECT_NAME} ${${PROJECT_NAME}_SRCS} ${${PROJECT_NAME}_HDRS})

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER "Tools")

TARGET_LINK_LIBRARIES(${PROJECT_NAME} comp)

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${COMP_INSTALL_DIR} COMPONENT tools)
