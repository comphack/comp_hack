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

PROJECT(sqlite3)

MESSAGE("** Configuring ${PROJECT_NAME} **")

REMOVE_COMPILER_FLAGS(AUTO -Wextra -Wconversion -Wsign-conversion)

# 'GetVersionExA': was declared deprecated
ADD_COMPILER_FLAGS(AUTO /wd4996)

ADD_LIBRARY(${PROJECT_NAME} STATIC sqlite3.c)

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER "Dependencies")
