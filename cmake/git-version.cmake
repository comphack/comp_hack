# This file is part of COMP_hack.
#
# Copyright (C) 2010-2017 COMP_hack Team <compomega@tutanota.com>
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

# Get the current working branch.
EXECUTE_PROCESS(
  COMMAND git rev-parse --abbrev-ref HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_BRANCH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get the committish.
EXECUTE_PROCESS(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMITTISH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get the author and date.
EXECUTE_PROCESS(
  COMMAND git log -1 --format=%an
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_AUTHOR
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get the author and date.
EXECUTE_PROCESS(
  COMMAND git log -1 --format=%ae
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_AUTHOR_EMAIL
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get the author and date.
EXECUTE_PROCESS(
  COMMAND git log -1 --format=%cd --date=short
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_DATE
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Get the commit description.
EXECUTE_PROCESS(
  COMMAND git log -1 --format=%s
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_DESCRIPTION
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

SET(GIT_BRANCH "R\"_c_esc_(${GIT_BRANCH})_c_esc_\"")
SET(GIT_COMMITTISH "R\"_c_esc_(${GIT_COMMITTISH})_c_esc_\"")
SET(GIT_AUTHOR "R\"_c_esc_(${GIT_AUTHOR})_c_esc_\"")
SET(GIT_AUTHOR_EMAIL "R\"_c_esc_(${GIT_AUTHOR_EMAIL})_c_esc_\"")
SET(GIT_DATE "R\"_c_esc_(${GIT_DATE})_c_esc_\"")
SET(GIT_DESCRIPTION "R\"_c_esc_(${GIT_DESCRIPTION})_c_esc_\"")

CONFIGURE_FILE("${SRC}" "${DST}")
