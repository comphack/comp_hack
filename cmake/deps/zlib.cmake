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

OPTION(USE_SYSTEM_ZLIB "Build with the system zlib library." OFF)

IF(USE_SYSTEM_ZLIB)
    FIND_PACKAGE(ZLIB)
ENDIF(USE_SYSTEM_ZLIB)

IF(ZLIB_FOUND)
    MESSAGE("-- Using system zlib")

    SET(ZLIB_INCLUDES "${ZLIB_INCLUDE_DIRS}")

    ADD_LIBRARY(zlib STATIC IMPORTED)

    SET_TARGET_PROPERTIES(zlib PROPERTIES
        IMPORTED_LOCATION "${ZLIB_LIBRARIES}")

    SET_TARGET_PROPERTIES(zlib PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${ZLIB_INCLUDES}")
ELSEIF(USE_EXTERNAL_BINARIES)
    MESSAGE("-- Using external binaries zlib")

    ADD_CUSTOM_TARGET(zlib-lib)

    SET(INSTALL_DIR "${CMAKE_SOURCE_DIR}/binaries/zlib")

    SET(ZLIB_INCLUDES "${INSTALL_DIR}/include")
    SET(ZLIB_LIBRARIES zlib)

    ADD_LIBRARY(zlib STATIC IMPORTED)

    IF(WIN32)
        SET_TARGET_PROPERTIES(zlib PROPERTIES
            IMPORTED_LOCATION_RELEASE "${INSTALL_DIR}/lib/zlibstatic.lib"
            IMPORTED_LOCATION_RELWITHDEBINFO "${INSTALL_DIR}/lib/zlibstatic_reldeb.lib"
            IMPORTED_LOCATION_DEBUG "${INSTALL_DIR}/lib/zlibstaticd.lib")
    ELSE()
        SET_TARGET_PROPERTIES(zlib PROPERTIES
            IMPORTED_LOCATION "${INSTALL_DIR}/lib/libz.a")
    ENDIF()

    SET_TARGET_PROPERTIES(zlib PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${ZLIB_INCLUDES}")
ELSE(ZLIB_FOUND)
    MESSAGE("-- Building external zlib")

    IF(EXISTS "${CMAKE_SOURCE_DIR}/deps/zlib.zip")
        SET(ZLIB_URL
            URL "${CMAKE_SOURCE_DIR}/deps/zlib.zip"
        )
    ELSEIF(GIT_DEPENDENCIES)
        SET(ZLIB_LIBRARY "${INSTALL_DIR}/lib/zlibstatic_reldeb.lib" PARENT_SCOPE)
        SET(ZLIB_URL
            GIT_REPOSITORY https://github.com/comphack/zlib.git
            GIT_TAG comp_hack
        )
    ELSE()
        SET(ZLIB_LIBRARY "${INSTALL_DIR}/lib/libz.a" PARENT_SCOPE)
        SET(ZLIB_URL
            URL https://github.com/comphack/zlib/archive/comp_hack-20180425.zip
            URL_HASH SHA1=41ef62fec86b9a4408d99c2e7ee1968a5e246e3b
        )
    ENDIF()

    ExternalProject_Add(
        zlib-lib

        ${ZLIB_URL}

        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/zlib
        CMAKE_ARGS ${CMAKE_RELWITHDEBINFO_OPTIONS} -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -DSKIP_INSTALL_FILES=ON -DUSE_STATIC_RUNTIME=${USE_STATIC_RUNTIME}

        # Dump output to a log instead of the screen.
        LOG_DOWNLOAD ON
        LOG_CONFIGURE ON
        LOG_BUILD ON
        LOG_INSTALL ON

        BUILD_BYPRODUCTS <INSTALL_DIR>/lib/libz.a
        BUILD_BYPRODUCTS <INSTALL_DIR>/lib/zlibstatic.lib
        BUILD_BYPRODUCTS <INSTALL_DIR>/lib/zlibstaticd.lib
        BUILD_BYPRODUCTS <INSTALL_DIR>/lib/zlibstatic_reldeb.lib
    )

    ExternalProject_Get_Property(zlib-lib INSTALL_DIR)

    SET_TARGET_PROPERTIES(zlib-lib PROPERTIES FOLDER "Dependencies")

    SET(ZLIB_INCLUDES "${INSTALL_DIR}/include")
    SET(ZLIB_LIBRARIES zlib)

    FILE(MAKE_DIRECTORY "${ZLIB_INCLUDES}")

    ADD_LIBRARY(zlib STATIC IMPORTED)
    ADD_DEPENDENCIES(zlib zlib-lib)

    IF(WIN32)
        SET_TARGET_PROPERTIES(zlib PROPERTIES
            IMPORTED_LOCATION_RELEASE "${INSTALL_DIR}/lib/zlibstatic.lib"
            IMPORTED_LOCATION_RELWITHDEBINFO "${INSTALL_DIR}/lib/zlibstatic_reldeb.lib"
            IMPORTED_LOCATION_DEBUG "${INSTALL_DIR}/lib/zlibstaticd.lib")
    ELSE()
        SET_TARGET_PROPERTIES(zlib PROPERTIES
            IMPORTED_LOCATION "${INSTALL_DIR}/lib/libz.a")
    ENDIF()

    SET_TARGET_PROPERTIES(zlib PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${ZLIB_INCLUDES}")
ENDIF(ZLIB_FOUND)
