# This file is part of COMP_hack.
#
# Copyright (C) 2010-2016 COMP_hack Team <compomega@tutanota.com>
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

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(libobjgen)

FIND_PACKAGE(Threads REQUIRED)

MESSAGE("** Configuring ${PROJECT_NAME} **")

# Add a target to generate API documentation.
FILE(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/api/${PROJECT_NAME})
#GENERATE_DOCS(doc/Doxyfile.in)

INCLUDE_DIRECTORIES(${ZLIB_INCLUDES})
INCLUDE_DIRECTORIES(${GTEST_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${LIBOBJGEN_INCLUDES})
INCLUDE_DIRECTORIES(${CASSANDRA_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${TINYXML2_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${OPENSSL_INCLUDE_DIR})

SET(${PROJECT_NAME}_SRCS
    src/Generator.cpp
    src/GeneratorFactory.cpp
    src/GeneratorHeader.cpp
    src/GeneratorSource.cpp
    src/MetaObject.cpp
    src/MetaVariable.cpp
    src/MetaVariableArray.cpp
    src/MetaVariableList.cpp
    src/MetaVariableReference.cpp
    src/MetaVariableString.cpp
    src/UUID.cpp
)

SET(${PROJECT_NAME}_HDRS
    src/Generator.h
    src/GeneratorFactory.h
    src/GeneratorHeader.h
    src/GeneratorSource.h
    src/MetaObject.h
    src/MetaVariable.h
    src/MetaVariableArray.h
    src/MetaVariableInt.h
    src/MetaVariableList.h
    src/MetaVariableReference.h
    src/MetaVariableString.h
    src/UUID.h
)

ADD_LIBRARY(objgen STATIC ${${PROJECT_NAME}_SRCS} ${${PROJECT_NAME}_HDRS})

TARGET_LINK_LIBRARIES(objgen ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES} tinyxml2
    cassandra uv)

IF(WIN32)
    TARGET_LINK_LIBRARIES(objgen advapi32 iphlpapi psapi shell32 userenv ws2_32)
ENDIF(WIN32)

# List of unit tests to add to CTest.
SET(${PROJECT_NAME}_TEST_SRCS
    UUID
)

IF(NOT BSD)
    # Add the unit tests.
    CREATE_GTESTS(LIBS objgen ${CMAKE_THREAD_LIBS_INIT} cassandra
        uv SRCS ${${PROJECT_NAME}_TEST_SRCS})
ENDIF(NOT BSD)
