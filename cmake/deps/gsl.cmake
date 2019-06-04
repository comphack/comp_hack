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

IF(USE_EXTERNAL_BINARIES)
    MESSAGE("-- Using external binaries GSL")

    ADD_CUSTOM_TARGET(gsl)

    SET(SOURCE_DIR "${CMAKE_SOURCE_DIR}/binaries/gsl")

    SET(GSL_INCLUDE_DIRS "${SOURCE_DIR}/include")
ELSE(USE_EXTERNAL_BINARIES)
    MESSAGE("-- Building external GSL")

    IF(EXISTS "${CMAKE_SOURCE_DIR}/deps/GSL.zip")
        SET(GSL_URL
            URL "${CMAKE_SOURCE_DIR}/deps/GSL.zip"
        )
    ELSEIF(GIT_DEPENDENCIES)
        SET(GSL_URL
            GIT_REPOSITORY https://github.com/Microsoft/GSL.git
            GIT_TAG master
        )
    ELSE()
        SET(GSL_URL
            URL https://github.com/Microsoft/GSL/archive/5905d2d77467d9daa18fe711b55e2db7a30fe3e3.zip
            URL_HASH SHA1=a2d2c6bfe101be3ef80d9c69e3361f164517e7b9
        )
    ENDIF()

    ExternalProject_Add(
        gsl

        ${GSL_URL}

        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gsl
        CMAKE_ARGS ${CMAKE_RELWITHDEBINFO_OPTIONS} -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>

        # Dump output to a log instead of the screen.
        LOG_DOWNLOAD ON
        LOG_CONFIGURE ON
        LOG_BUILD ON
        LOG_INSTALL ON

        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
    )

    ExternalProject_Get_Property(gsl SOURCE_DIR)
    ExternalProject_Get_Property(gsl INSTALL_DIR)

    SET_TARGET_PROPERTIES(gsl PROPERTIES FOLDER "Dependencies")

    SET(GSL_INCLUDE_DIRS "${SOURCE_DIR}/include")

    FILE(MAKE_DIRECTORY "${GSL_INCLUDE_DIRS}")
ENDIF(USE_EXTERNAL_BINARIES)
