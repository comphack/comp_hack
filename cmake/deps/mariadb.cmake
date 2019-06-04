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

OPTION(USE_SYSTEM_MARIADB "Build with the system MariaDB client library." OFF)

IF(USE_SYSTEM_MARIADB)
    FIND_PACKAGE(MariaDBClient)
ENDIF(USE_SYSTEM_MARIADB)

IF(MariaDBClient_FOUND)
    MESSAGE("-- Using system MariaDB Client")

    ADD_CUSTOM_TARGET(mariadb)

    ADD_LIBRARY(mariadbclient STATIC IMPORTED)
    SET_TARGET_PROPERTIES(mariadbclient PROPERTIES IMPORTED_LOCATION
        "${MariaDBClient_LIBRARIES}")
    SET_TARGET_PROPERTIES(mariadbclient PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${MariaDBClient_INCLUDE_DIRS}")
ELSEIF(USE_EXTERNAL_BINARIES)
    MESSAGE("-- Using external binaries MariaDB Client")

    ADD_CUSTOM_TARGET(mariadb)

    SET(INSTALL_DIR "${CMAKE_SOURCE_DIR}/binaries/mariadb")

    SET(MARIADB_INCLUDE_DIRS "${INSTALL_DIR}/include/mariadb")

    ADD_LIBRARY(mariadbclient STATIC IMPORTED)

    IF(WIN32)
        SET_TARGET_PROPERTIES(mariadbclient PROPERTIES
            IMPORTED_LOCATION_RELEASE "${INSTALL_DIR}/lib/mariadb/${CMAKE_STATIC_LIBRARY_PREFIX}mariadbclient${CMAKE_STATIC_LIBRARY_SUFFIX}"
            IMPORTED_LOCATION_RELWITHDEBINFO "${INSTALL_DIR}/lib/mariadb/${CMAKE_STATIC_LIBRARY_PREFIX}mariadbclient_reldeb${CMAKE_STATIC_LIBRARY_SUFFIX}"
            IMPORTED_LOCATION_DEBUG "${INSTALL_DIR}/lib/mariadb/${CMAKE_STATIC_LIBRARY_PREFIX}mariadbclientd${CMAKE_STATIC_LIBRARY_SUFFIX}")
    ELSE()
        SET_TARGET_PROPERTIES(mariadbclient PROPERTIES IMPORTED_LOCATION
            "${INSTALL_DIR}/lib/mariadb/${CMAKE_STATIC_LIBRARY_PREFIX}mariadbclient${CMAKE_STATIC_LIBRARY_SUFFIX}")
    ENDIF()

    SET_TARGET_PROPERTIES(mariadbclient PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${MARIADB_INCLUDE_DIRS}")
ELSE(MariaDBClient_FOUND)
    MESSAGE("-- Building external MariaDB Client")

    IF(EXISTS "${CMAKE_SOURCE_DIR}/deps/mariadb.zip")
        SET(MARIADB_URL
            URL "${CMAKE_SOURCE_DIR}/deps/mariadb.zip"
        )
    ELSEIF(GIT_DEPENDENCIES)
        SET(MARIADB_URL
            GIT_REPOSITORY https://github.com/comphack/mariadb.git
            GIT_TAG comp_hack
        )
    ELSE()
        SET(MARIADB_URL
            URL https://github.com/comphack/mariadb/archive/comp_hack-20190603.zip
            URL_HASH SHA1=4e93b6dfe3872223becf0d3ebf27aadf9aa05e17
        )
    ENDIF()

    ExternalProject_Add(
        mariadb

        ${MARIADB_URL}

        DEPENDS openssl

        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/mariadb
        CMAKE_ARGS ${CMAKE_RELWITHDEBINFO_OPTIONS} -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> "-DCMAKE_CXX_FLAGS=-std=c++11 ${SPECIAL_COMPILER_FLAGS}" "-DOPENSSL_ROOT_DIR=${OPENSSL_ROOT_DIR}" -DUSE_STATIC_RUNTIME=${USE_STATIC_RUNTIME} -DCMAKE_DEBUG_POSTFIX=d -DWITH_OPENSSL=ON -DUSE_SYSTEM_OPENSSL=${USE_SYSTEM_OPENSSL}

        # Dump output to a log instead of the screen.
        LOG_DOWNLOAD ON
        LOG_CONFIGURE ON
        LOG_BUILD ON
        LOG_INSTALL ON

        BUILD_BYPRODUCTS <INSTALL_DIR>/lib/mariadb/${CMAKE_STATIC_LIBRARY_PREFIX}mariadbclient${CMAKE_STATIC_LIBRARY_SUFFIX}
        BUILD_BYPRODUCTS <INSTALL_DIR>/lib/mariadb/${CMAKE_STATIC_LIBRARY_PREFIX}mariadbclientd${CMAKE_STATIC_LIBRARY_SUFFIX}
        BUILD_BYPRODUCTS <INSTALL_DIR>/lib/mariadb/${CMAKE_STATIC_LIBRARY_PREFIX}mariadbclient_reldeb${CMAKE_STATIC_LIBRARY_SUFFIX}
    )

    ExternalProject_Get_Property(mariadb INSTALL_DIR)

    SET_TARGET_PROPERTIES(mariadb PROPERTIES FOLDER "Dependencies")

    SET(MARIADB_INCLUDE_DIRS "${INSTALL_DIR}/include/mariadb")

    FILE(MAKE_DIRECTORY "${MARIADB_INCLUDE_DIRS}")

    ADD_LIBRARY(mariadbclient STATIC IMPORTED)
    ADD_DEPENDENCIES(mariadbclient mariadb)

    IF(WIN32)
        SET_TARGET_PROPERTIES(mariadbclient PROPERTIES
            IMPORTED_LOCATION_RELEASE "${INSTALL_DIR}/lib/mariadb/${CMAKE_STATIC_LIBRARY_PREFIX}mariadbclient${CMAKE_STATIC_LIBRARY_SUFFIX}"
            IMPORTED_LOCATION_RELWITHDEBINFO "${INSTALL_DIR}/lib/mariadb/${CMAKE_STATIC_LIBRARY_PREFIX}mariadbclient_reldeb${CMAKE_STATIC_LIBRARY_SUFFIX}"
            IMPORTED_LOCATION_DEBUG "${INSTALL_DIR}/lib/mariadb/${CMAKE_STATIC_LIBRARY_PREFIX}mariadbclientd${CMAKE_STATIC_LIBRARY_SUFFIX}")
    ELSE()
        SET_TARGET_PROPERTIES(mariadbclient PROPERTIES IMPORTED_LOCATION
            "${INSTALL_DIR}/lib/mariadb/${CMAKE_STATIC_LIBRARY_PREFIX}mariadbclient${CMAKE_STATIC_LIBRARY_SUFFIX}")
    ENDIF()

    SET_TARGET_PROPERTIES(mariadbclient PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${MARIADB_INCLUDE_DIRS}")
ENDIF(MariaDBClient_FOUND)
