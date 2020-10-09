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
    MESSAGE("-- Using external binaries asio")

    ADD_CUSTOM_TARGET(asio)

    SET(INSTALL_DIR "${BINARIES_PATH}/asio")

    SET(ASIO_INCLUDE_DIRS "${INSTALL_DIR}/include")
ELSE(USE_EXTERNAL_BINARIES)
    MESSAGE("-- Building external asio")

    IF(EXISTS "${CMAKE_SOURCE_DIR}/deps/asio.zip")
        SET(ASIO_URL
            URL "${CMAKE_SOURCE_DIR}/deps/asio.zip"
        )
    ELSEIF(GIT_DEPENDENCIES)
        SET(ASIO_URL
            GIT_REPOSITORY https://github.com/comphack/asio.git
            GIT_TAG comp_hack
        )
    ELSE()
        SET(ASIO_URL
            URL https://github.com/comphack/asio/archive/comp_hack-20200402.zip
            URL_HASH SHA1=26d1af5e88ae4e0e93f64d614cd5f59ba13a024d
        )
    ENDIF()

    ExternalProject_Add(
        asio

        ${ASIO_URL}

        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/asio
        CMAKE_ARGS ${CMAKE_RELWITHDEBINFO_OPTIONS} -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR> "-DCMAKE_CXX_FLAGS=-std=c++11 ${SPECIAL_COMPILER_FLAGS}" -DUSE_STATIC_RUNTIME=${USE_STATIC_RUNTIME} -DCMAKE_DEBUG_POSTFIX=d

        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""

        # Dump output to a log instead of the screen.
        LOG_DOWNLOAD ON
        LOG_CONFIGURE ON
        LOG_BUILD ON
        LOG_INSTALL ON
    )

    ExternalProject_Get_Property(asio INSTALL_DIR)

    SET_TARGET_PROPERTIES(asio PROPERTIES FOLDER "Dependencies")

    SET(ASIO_INCLUDE_DIRS "${INSTALL_DIR}/src/asio/asio/include")

    FILE(MAKE_DIRECTORY "${ASIO_INCLUDE_DIRS}")
ENDIF(USE_EXTERNAL_BINARIES)
