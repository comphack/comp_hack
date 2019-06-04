# This file is part of COMP_hack.
#
# Copyright (C) 2010-2019 COMP_hack Team <compomega@tutanota.com>
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

# Enable the ExternalProject CMake module.
INCLUDE(ExternalProject)

OPTION(GIT_DEPENDENCIES "Download dependencies from Git instead." OFF)

IF(WIN32)
    SET(CMAKE_RELWITHDEBINFO_OPTIONS -DCMAKE_RELWITHDEBINFO_POSTFIX=_reldeb)
ENDIF(WIN32)

IF(NOT UPDATER_ONLY)
INCLUDE("${CMAKE_SOURCE_DIR}/cmake/deps/gsl.cmake")
ENDIF(NOT UPDATER_ONLY)

INCLUDE("${CMAKE_SOURCE_DIR}/cmake/deps/zlib.cmake")

IF(NOT UPDATER_ONLY)
INCLUDE("${CMAKE_SOURCE_DIR}/cmake/deps/openssl.cmake")
INCLUDE("${CMAKE_SOURCE_DIR}/cmake/deps/mariadb.cmake")
INCLUDE("${CMAKE_SOURCE_DIR}/cmake/deps/ttvfs.cmake")
INCLUDE("${CMAKE_SOURCE_DIR}/cmake/deps/physfs.cmake")
INCLUDE("${CMAKE_SOURCE_DIR}/cmake/deps/civet.cmake")
INCLUDE("${CMAKE_SOURCE_DIR}/cmake/deps/squirrel3.cmake")
INCLUDE("${CMAKE_SOURCE_DIR}/cmake/deps/asio.cmake")
INCLUDE("${CMAKE_SOURCE_DIR}/cmake/deps/tinyxml2.cmake")
INCLUDE("${CMAKE_SOURCE_DIR}/cmake/deps/googletest.cmake")
INCLUDE("${CMAKE_SOURCE_DIR}/cmake/deps/jsonbox.cmake")
ENDIF(NOT UPDATER_ONLY)
