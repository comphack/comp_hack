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

# Search for git.
FIND_PACKAGE(Git)

IF(GIT_FOUND)
  # Get the current working branch.
  EXECUTE_PROCESS(
    COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE GIT_STATUS
    OUTPUT_VARIABLE GIT_BRANCH
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  IF(0 EQUAL ${GIT_STATUS})
    # Get the committish.
    EXECUTE_PROCESS(
      COMMAND ${GIT_EXECUTABLE} log -1 --format=%h
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE GIT_COMMITTISH
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Get the author and date.
    EXECUTE_PROCESS(
      COMMAND ${GIT_EXECUTABLE} log -1 --format=%an
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE GIT_AUTHOR
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Get the author and date.
    EXECUTE_PROCESS(
      COMMAND ${GIT_EXECUTABLE} log -1 --format=%ae
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE GIT_AUTHOR_EMAIL
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Get the author and date.
    EXECUTE_PROCESS(
      COMMAND ${GIT_EXECUTABLE} log -1 --format=%cd --date=short
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE GIT_DATE
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Get the commit description.
    EXECUTE_PROCESS(
      COMMAND ${GIT_EXECUTABLE} log -1 --format=%s
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE GIT_DESCRIPTION
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Get the remote.
    EXECUTE_PROCESS(
      COMMAND ${GIT_EXECUTABLE} remote
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE GIT_REMOTE
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Use the first one in the list.
    STRING(REPLACE "\n" ";" GIT_REMOTE "${GIT_REMOTE}")
    LIST(GET GIT_REMOTE 0 GIT_REMOTE)

    # Get the symbolic ref for HEAD
    EXECUTE_PROCESS(
      COMMAND ${GIT_EXECUTABLE} symbolic-ref -q HEAD
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE GIT_SYMREF_HEAD
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Get the tracking remote.
    EXECUTE_PROCESS(
      COMMAND ${GIT_EXECUTABLE} for-each-ref "--format=%(upstream:short)" "${GIT_SYMREF_HEAD}"
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      RESULT_VARIABLE GIT_TRACKING_REMOTE_STATUS
      OUTPUT_VARIABLE GIT_TRACKING_REMOTE
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
    )
    STRING(LENGTH "${GIT_TRACKING_REMOTE}" GIT_TRACKING_REMOTE_LENGTH)

    # If we found the tracking remote use it.
    IF(0 EQUAL GIT_TRACKING_REMOTE_STATUS AND NOT 0 EQUAL ${GIT_TRACKING_REMOTE_LENGTH})
      STRING(REPLACE "/" ";" GIT_TRACKING_REMOTE "${GIT_TRACKING_REMOTE}")
      LIST(GET GIT_TRACKING_REMOTE 0 GIT_REMOTE)
    ENDIF()

    # Get the remote URL.
    EXECUTE_PROCESS(
      COMMAND ${GIT_EXECUTABLE} remote get-url "${GIT_REMOTE}"
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE GIT_REMOTE_URL
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    SET(GIT_BRANCH "R\"_c_esc_(${GIT_BRANCH})_c_esc_\"")
    SET(GIT_COMMITTISH "R\"_c_esc_(${GIT_COMMITTISH})_c_esc_\"")
    SET(GIT_AUTHOR "R\"_c_esc_(${GIT_AUTHOR})_c_esc_\"")
    SET(GIT_AUTHOR_EMAIL "R\"_c_esc_(${GIT_AUTHOR_EMAIL})_c_esc_\"")
    SET(GIT_DATE "R\"_c_esc_(${GIT_DATE})_c_esc_\"")
    SET(GIT_DESCRIPTION "R\"_c_esc_(${GIT_DESCRIPTION})_c_esc_\"")
    SET(GIT_REMOTE "R\"_c_esc_(${GIT_REMOTE})_c_esc_\"")
    SET(GIT_REMOTE_URL "R\"_c_esc_(${GIT_REMOTE_URL})_c_esc_\"")

    CONFIGURE_FILE("${SRC}" "${DST}")
  ELSE()
    FILE(WRITE "${DST}" "// Source code is not a Git checkout.")
  ENDIF()
ELSE()
  FILE(WRITE "${DST}" "// Git was not found.")
ENDIF()
