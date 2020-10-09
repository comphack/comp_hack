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

OPTION(USE_SYSTEM_TINYXML2 "Build with the system TinyXML2 library." OFF)

IF(USE_SYSTEM_TINYXML2)
    FIND_PACKAGE(TinyXML2)
ENDIF(USE_SYSTEM_TINYXML2)

IF(TINYXML2_FOUND)
    MESSAGE("-- Using system TinyXML2")

    ADD_CUSTOM_TARGET(tinyxml2-ex)

    ADD_LIBRARY(tinyxml2 STATIC IMPORTED)
    SET_TARGET_PROPERTIES(tinyxml2 PROPERTIES IMPORTED_LOCATION
        "${TINYXML2_LIBRARIES}")
    SET_TARGET_PROPERTIES(tinyxml2 PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${TINYXML2_INCLUDE_DIRS}")
ELSEIF(USE_EXTERNAL_BINARIES)
    MESSAGE("-- Using external binaries TinyXML2")

    ADD_CUSTOM_TARGET(tinyxml2-ex)

    SET(INSTALL_DIR "${BINARIES_PATH}/tinyxml2")

    SET(TINYXML2_INCLUDE_DIRS "${INSTALL_DIR}/include")

    ADD_LIBRARY(tinyxml2 STATIC IMPORTED)

    IF(WIN32)
        SET_TARGET_PROPERTIES(tinyxml2 PROPERTIES
            IMPORTED_LOCATION_RELEASE "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tinyxml2${CMAKE_STATIC_LIBRARY_SUFFIX}"
            IMPORTED_LOCATION_RELWITHDEBINFO "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tinyxml2_reldeb${CMAKE_STATIC_LIBRARY_SUFFIX}"
            IMPORTED_LOCATION_DEBUG "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tinyxml2d${CMAKE_STATIC_LIBRARY_SUFFIX}")
    ELSE()
        SET_TARGET_PROPERTIES(tinyxml2 PROPERTIES IMPORTED_LOCATION
            "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tinyxml2${CMAKE_STATIC_LIBRARY_SUFFIX}")
    ENDIF()

    SET_TARGET_PROPERTIES(tinyxml2 PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${TINYXML2_INCLUDE_DIRS}")
ELSE(TINYXML2_FOUND)
    MESSAGE("-- Building external TinyXML2")

    IF(EXISTS "${CMAKE_SOURCE_DIR}/deps/tinyxml2.zip")
        SET(TINYXML2_URL
            URL "${CMAKE_SOURCE_DIR}/deps/tinyxml2.zip"
        )
    ELSEIF(GIT_DEPENDENCIES)
        SET(TINYXML2_URL
            GIT_REPOSITORY https://github.com/comphack/tinyxml2.git
            GIT_TAG comp_hack
        )
    ELSE()
        SET(TINYXML2_URL
            URL https://github.com/comphack/tinyxml2/archive/comp_hack-20200318.zip
            URL_HASH SHA1=bccb54ff37d0076424da4a28a302515eef2f3981

        )
    ENDIF()

    ExternalProject_Add(
        tinyxml2-ex

        ${TINYXML2_URL}

        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/tinyxml2
        CMAKE_ARGS ${CMAKE_RELWITHDEBINFO_OPTIONS} -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -DBUILD_SHARED_LIBS=OFF "-DCMAKE_CXX_FLAGS=-std=c++11 ${SPECIAL_COMPILER_FLAGS}" -DUSE_STATIC_RUNTIME=${USE_STATIC_RUNTIME} -DCMAKE_DEBUG_POSTFIX=d -DCMAKE_INSTALL_LIBDIR=lib

        # Dump output to a log instead of the screen.
        LOG_DOWNLOAD ON
        LOG_CONFIGURE ON
        LOG_BUILD ON
        LOG_INSTALL ON

        BUILD_BYPRODUCTS <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tinyxml2${CMAKE_STATIC_LIBRARY_SUFFIX}
        BUILD_BYPRODUCTS <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tinyxml2d${CMAKE_STATIC_LIBRARY_SUFFIX}
        BUILD_BYPRODUCTS <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tinyxml2_reldeb${CMAKE_STATIC_LIBRARY_SUFFIX}
    )

    ExternalProject_Get_Property(tinyxml2-ex INSTALL_DIR)

    SET_TARGET_PROPERTIES(tinyxml2-ex PROPERTIES FOLDER "Dependencies")

    SET(TINYXML2_INCLUDE_DIRS "${INSTALL_DIR}/include")

    FILE(MAKE_DIRECTORY "${TINYXML2_INCLUDE_DIRS}")

    ADD_LIBRARY(tinyxml2 STATIC IMPORTED)
    ADD_DEPENDENCIES(tinyxml2 tinyxml2-ex)

    IF(WIN32)
        SET_TARGET_PROPERTIES(tinyxml2 PROPERTIES
            IMPORTED_LOCATION_RELEASE "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tinyxml2${CMAKE_STATIC_LIBRARY_SUFFIX}"
            IMPORTED_LOCATION_RELWITHDEBINFO "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tinyxml2_reldeb${CMAKE_STATIC_LIBRARY_SUFFIX}"
            IMPORTED_LOCATION_DEBUG "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tinyxml2d${CMAKE_STATIC_LIBRARY_SUFFIX}")
    ELSE()
        SET_TARGET_PROPERTIES(tinyxml2 PROPERTIES IMPORTED_LOCATION
            "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}tinyxml2${CMAKE_STATIC_LIBRARY_SUFFIX}")
    ENDIF()

    SET_TARGET_PROPERTIES(tinyxml2 PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${TINYXML2_INCLUDE_DIRS}")
ENDIF(TINYXML2_FOUND)
