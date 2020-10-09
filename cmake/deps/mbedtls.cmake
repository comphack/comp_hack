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

IF(USE_EXTERNAL_BINARIES)
    MESSAGE("-- Using external binaries mbedtls")

    ADD_CUSTOM_TARGET(mbedtls-ex)

    SET(INSTALL_DIR "${BINARIES_PATH}/mbedtls")

    SET(MBEDTLS_INCLUDE_DIRS "${INSTALL_DIR}/include")

    ADD_LIBRARY(mbedcrypto STATIC IMPORTED)

    IF(WIN32)
        SET_TARGET_PROPERTIES(mbedcrypto PROPERTIES
            IMPORTED_LOCATION_RELEASE "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}mbecrypto${CMAKE_STATIC_LIBRARY_SUFFIX}"
            IMPORTED_LOCATION_RELWITHDEBINFO "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}mbedcrypto_reldeb${CMAKE_STATIC_LIBRARY_SUFFIX}"
            IMPORTED_LOCATION_DEBUG "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}mbedcryptod${CMAKE_STATIC_LIBRARY_SUFFIX}")
    ELSE()
        SET_TARGET_PROPERTIES(mbedcrypto PROPERTIES IMPORTED_LOCATION
            "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}mbedcrypto${CMAKE_STATIC_LIBRARY_SUFFIX}")
    ENDIF()

    SET_TARGET_PROPERTIES(mbedcrypto PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${MBEDTLS_INCLUDE_DIRS}")
ELSE(USE_EXTERNAL_BINARIES)
    MESSAGE("-- Building external mbedtls")

    IF(EXISTS "${CMAKE_SOURCE_DIR}/deps/mbedtls.zip")
        SET(MBEDTLS_URL
            URL "${CMAKE_SOURCE_DIR}/deps/mbedtls.zip"
        )
    ELSEIF(GIT_DEPENDENCIES)
        SET(MBEDTLS_URL
            GIT_REPOSITORY https://github.com/comphack/mbedtls.git
            GIT_TAG comp_hack
        )
    ELSE()
        SET(MBEDTLS_URL
            URL https://github.com/comphack/mbedtls/archive/comp_hack-20190710.zip
            URL_HASH SHA1=2986742b0dc65da1aae5121f7e16d5ac04c3aba2
        )
    ENDIF()

    ExternalProject_Add(
        mbedtls-ex

        ${MBEDTLS_URL}

        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/mbedtls
        CMAKE_ARGS ${CMAKE_RELWITHDEBINFO_OPTIONS} -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> "-DCMAKE_CXX_FLAGS=-std=c++11 ${SPECIAL_COMPILER_FLAGS}" -DUSE_STATIC_RUNTIME=${USE_STATIC_RUNTIME} -DCMAKE_DEBUG_POSTFIX=d -DENABLE_PROGRAMS=OFF

        # Dump output to a log instead of the screen.
        LOG_DOWNLOAD ON
        LOG_CONFIGURE ON
        LOG_BUILD ON
        LOG_INSTALL ON

        BUILD_BYPRODUCTS <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}mbedcrypto${CMAKE_STATIC_LIBRARY_SUFFIX}
        BUILD_BYPRODUCTS <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}mbedcryptod${CMAKE_STATIC_LIBRARY_SUFFIX}
        BUILD_BYPRODUCTS <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}mbedcrypto_reldeb${CMAKE_STATIC_LIBRARY_SUFFIX}
    )

    ExternalProject_Get_Property(mbedtls-ex INSTALL_DIR)

    SET_TARGET_PROPERTIES(mbedtls-ex PROPERTIES FOLDER "Dependencies")

    SET(MBEDTLS_INCLUDE_DIRS "${INSTALL_DIR}/include")

    FILE(MAKE_DIRECTORY "${MBEDTLS_INCLUDE_DIRS}")

    ADD_LIBRARY(mbedcrypto STATIC IMPORTED)
    ADD_DEPENDENCIES(mbedcrypto mbedtls-ex)

    IF(WIN32)
        SET_TARGET_PROPERTIES(mbedcrypto PROPERTIES
            IMPORTED_LOCATION_RELEASE "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}mbedcrypto${CMAKE_STATIC_LIBRARY_SUFFIX}"
            IMPORTED_LOCATION_RELWITHDEBINFO "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}mbedcrypto_reldeb${CMAKE_STATIC_LIBRARY_SUFFIX}"
            IMPORTED_LOCATION_DEBUG "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}mbedcryptod${CMAKE_STATIC_LIBRARY_SUFFIX}")
    ELSE()
        SET_TARGET_PROPERTIES(mbedcrypto PROPERTIES IMPORTED_LOCATION
            "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}mbedcrypto${CMAKE_STATIC_LIBRARY_SUFFIX}")
    ENDIF()

    SET_TARGET_PROPERTIES(mbedcrypto PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${MBEDTLS_INCLUDE_DIRS}")
ENDIF(USE_EXTERNAL_BINARIES)
