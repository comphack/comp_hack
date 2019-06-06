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

OPTION(USE_SYSTEM_OPENSSL "Build with the system OpenSSL library." OFF)

IF(USE_SYSTEM_OPENSSL)
    IF(WIN32)
        SET(OPENSSL_USE_STATIC_LIBS TRUE)

        IF(USE_STATIC_RUNTIME)
            SET(OPENSSL_MSVC_STATIC_RT TRUE)
        ENDIF(USE_STATIC_RUNTIME)
    ENDIF(WIN32)

    FIND_PACKAGE(OpenSSL)
ENDIF(USE_SYSTEM_OPENSSL)

IF(OPENSSL_FOUND)
    MESSAGE("-- Using system OpenSSL")

    ADD_CUSTOM_TARGET(openssl)

    ADD_LIBRARY(ssl STATIC IMPORTED)
    SET_TARGET_PROPERTIES(ssl PROPERTIES IMPORTED_LOCATION
        "${OPENSSL_SSL_LIBRARY}")
    SET_TARGET_PROPERTIES(ssl PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}")

    ADD_LIBRARY(crypto STATIC IMPORTED)
    SET_TARGET_PROPERTIES(crypto PROPERTIES IMPORTED_LOCATION
        "${OPENSSL_CRYPTO_LIBRARY}")
    SET_TARGET_PROPERTIES(crypto PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}")
ELSEIF(USE_EXTERNAL_BINARIES)
    MESSAGE("-- Using external binaries OpenSSL")

    ADD_CUSTOM_TARGET(openssl)

    SET(INSTALL_DIR "${CMAKE_SOURCE_DIR}/binaries/openssl")

    SET(OPENSSL_INCLUDE_DIR "${INSTALL_DIR}/include")
    SET(OPENSSL_ROOT_DIR "${INSTALL_DIR}")

    IF(WIN32)
        SET(OPENSSL_LIBRARIES ssleay32 libeay32 crypt32)
    ELSE()
        SET(OPENSSL_LIBRARIES ssl crypto)
    ENDIF()

    IF(WIN32)
        ADD_LIBRARY(ssleay32 STATIC IMPORTED)
        SET_TARGET_PROPERTIES(ssleay32 PROPERTIES
            IMPORTED_LOCATION_RELEASE "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}ssleay32${CMAKE_STATIC_LIBRARY_SUFFIX}"
            IMPORTED_LOCATION_RELWITHDEBINFO "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}ssleay32_reldeb${CMAKE_STATIC_LIBRARY_SUFFIX}"
            IMPORTED_LOCATION_DEBUG "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}ssleay32d${CMAKE_STATIC_LIBRARY_SUFFIX}")

        SET_TARGET_PROPERTIES(ssleay32 PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}")

        ADD_LIBRARY(libeay32 STATIC IMPORTED)
        SET_TARGET_PROPERTIES(libeay32 PROPERTIES
            IMPORTED_LOCATION_RELEASE "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}libeay32${CMAKE_STATIC_LIBRARY_SUFFIX}"
            IMPORTED_LOCATION_RELWITHDEBINFO "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}libeay32_reldeb${CMAKE_STATIC_LIBRARY_SUFFIX}"
            IMPORTED_LOCATION_DEBUG "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}libeay32d${CMAKE_STATIC_LIBRARY_SUFFIX}")

        SET_TARGET_PROPERTIES(libeay32 PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}")
    ELSE()
        ADD_LIBRARY(ssl STATIC IMPORTED)
        SET_TARGET_PROPERTIES(ssl PROPERTIES IMPORTED_LOCATION
            "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}ssl${CMAKE_STATIC_LIBRARY_SUFFIX}")

        SET_TARGET_PROPERTIES(ssl PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}")

        ADD_LIBRARY(crypto STATIC IMPORTED)
        SET_TARGET_PROPERTIES(crypto PROPERTIES IMPORTED_LOCATION
            "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}crypto${CMAKE_STATIC_LIBRARY_SUFFIX}")

        SET_TARGET_PROPERTIES(crypto PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}")
    ENDIF()
ELSE(OPENSSL_FOUND)
    MESSAGE("-- Building external OpenSSL")

    IF(EXISTS "${CMAKE_SOURCE_DIR}/deps/openssl.zip")
        SET(OPENSSL_URL
            URL "${CMAKE_SOURCE_DIR}/deps/openssl.zip"
        )
    ELSEIF(GIT_DEPENDENCIES)
        SET(OPENSSL_URL
            GIT_REPOSITORY https://github.com/comphack/openssl.git
            GIT_TAG comp_hack
        )
    ELSE()
        SET(OPENSSL_URL
            URL https://github.com/comphack/openssl/archive/comp_hack-20180424.zip
            URL_HASH SHA1=0ac698894a8d9566a8d7982e32869252dc11d18b
        )
    ENDIF()

    ExternalProject_Add(
        openssl

        ${OPENSSL_URL}

        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/openssl
        CMAKE_ARGS ${CMAKE_RELWITHDEBINFO_OPTIONS} -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> -DUSE_STATIC_RUNTIME=${USE_STATIC_RUNTIME} -DCMAKE_DEBUG_POSTFIX=d -DBUILD_VALGRIND_FRIENDLY=${BUILD_VALGRIND_FRIENDLY}

        # Dump output to a log instead of the screen.
        LOG_DOWNLOAD ON
        LOG_CONFIGURE ON
        LOG_BUILD ON
        LOG_INSTALL ON

        BUILD_BYPRODUCTS <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}ssl${CMAKE_STATIC_LIBRARY_SUFFIX}
        BUILD_BYPRODUCTS <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}crypto${CMAKE_STATIC_LIBRARY_SUFFIX}

        BUILD_BYPRODUCTS <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}ssleay32${CMAKE_STATIC_LIBRARY_SUFFIX}
        BUILD_BYPRODUCTS <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}libeay32${CMAKE_STATIC_LIBRARY_SUFFIX}

        BUILD_BYPRODUCTS <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}ssleay32d${CMAKE_STATIC_LIBRARY_SUFFIX}
        BUILD_BYPRODUCTS <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}libeay32d${CMAKE_STATIC_LIBRARY_SUFFIX}

        BUILD_BYPRODUCTS <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}ssleay32_reldeb${CMAKE_STATIC_LIBRARY_SUFFIX}
        BUILD_BYPRODUCTS <INSTALL_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}libeay32_reldeb${CMAKE_STATIC_LIBRARY_SUFFIX}
    )

    ExternalProject_Get_Property(openssl INSTALL_DIR)

    SET_TARGET_PROPERTIES(openssl PROPERTIES FOLDER "Dependencies")

    SET(OPENSSL_INCLUDE_DIR "${INSTALL_DIR}/include")
    SET(OPENSSL_ROOT_DIR "${INSTALL_DIR}")

    FILE(MAKE_DIRECTORY "${OPENSSL_INCLUDE_DIR}")

    IF(WIN32)
        SET(OPENSSL_LIBRARIES ssleay32 libeay32 crypt32)
    ELSE()
        SET(OPENSSL_LIBRARIES ssl crypto)
    ENDIF()

    IF(WIN32)
        ADD_LIBRARY(ssleay32 STATIC IMPORTED)
        ADD_DEPENDENCIES(ssleay32 openssl)
        SET_TARGET_PROPERTIES(ssleay32 PROPERTIES
            IMPORTED_LOCATION_RELEASE "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}ssleay32${CMAKE_STATIC_LIBRARY_SUFFIX}"
            IMPORTED_LOCATION_RELWITHDEBINFO "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}ssleay32_reldeb${CMAKE_STATIC_LIBRARY_SUFFIX}"
            IMPORTED_LOCATION_DEBUG "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}ssleay32d${CMAKE_STATIC_LIBRARY_SUFFIX}")

        SET_TARGET_PROPERTIES(ssleay32 PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}")

        ADD_LIBRARY(libeay32 STATIC IMPORTED)
        ADD_DEPENDENCIES(libeay32 openssl)
        SET_TARGET_PROPERTIES(libeay32 PROPERTIES
            IMPORTED_LOCATION_RELEASE "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}libeay32${CMAKE_STATIC_LIBRARY_SUFFIX}"
            IMPORTED_LOCATION_RELWITHDEBINFO "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}libeay32_reldeb${CMAKE_STATIC_LIBRARY_SUFFIX}"
            IMPORTED_LOCATION_DEBUG "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}libeay32d${CMAKE_STATIC_LIBRARY_SUFFIX}")

        SET_TARGET_PROPERTIES(libeay32 PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}")
    ELSE()
        ADD_LIBRARY(ssl STATIC IMPORTED)
        ADD_DEPENDENCIES(ssl openssl)
        SET_TARGET_PROPERTIES(ssl PROPERTIES IMPORTED_LOCATION
            "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}ssl${CMAKE_STATIC_LIBRARY_SUFFIX}")

        SET_TARGET_PROPERTIES(ssl PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}")

        ADD_LIBRARY(crypto STATIC IMPORTED)
        ADD_DEPENDENCIES(crypto openssl)
        SET_TARGET_PROPERTIES(crypto PROPERTIES IMPORTED_LOCATION
            "${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}crypto${CMAKE_STATIC_LIBRARY_SUFFIX}")

        SET_TARGET_PROPERTIES(crypto PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${OPENSSL_INCLUDE_DIR}")
    ENDIF()
ENDIF(OPENSSL_FOUND)
