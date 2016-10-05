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

PROJECT(libcomp)

FIND_PACKAGE(Threads REQUIRED)

MESSAGE("** Configuring ${PROJECT_NAME} **")

# Add a target to generate API documentation.
FILE(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/api/libcomp)
#GENERATE_DOCS(doc/Doxyfile.in)

# Add a directory to put the objgen output into.
FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/objgen)

ADD_DEFINITIONS(${SQRAT_DEFINES})

INCLUDE_DIRECTORIES(${ZLIB_INCLUDES})
INCLUDE_DIRECTORIES(${LIBCOMP_INCLUDES})
INCLUDE_DIRECTORIES(${ASIO_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${GTEST_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${SQUIRREL_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${SQRAT_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${TTVFS_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${SQLITE3_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${LIBUV_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${CASSANDRA_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${TINYXML2_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${OPENSSL_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

SET(SYSTEM_LIBRARIES "")

IF(BSD)
FIND_PACKAGE(ExecInfo REQUIRED)

SET(SYSTEM_LIBRARIES ${SYSTEM_LIBRARIES} ${EXECINFO_LIBRARY})
ENDIF(BSD)

SET(${PROJECT_NAME}_SRCS
    src/Compress.cpp
    src/Convert.cpp
    src/CString.cpp
    src/Database.cpp
    src/DatabaseCassandra.cpp
    src/DatabaseQuery.cpp
    src/DatabaseQueryCassandra.cpp
    src/DatabaseSQLite3.cpp
    src/Decrypt.cpp
    #src/EngineLocker.cpp
    src/Exception.cpp
    src/InternalConnection.cpp
    src/InternalServer.cpp
    src/LobbyConnection.cpp
    src/Log.cpp
    #src/MemoryFile.cpp
    src/MessageEncrypted.cpp
    src/MessagePacket.cpp
    src/Object.cpp
    src/Packet.cpp
    src/PacketException.cpp
    #src/PacketScript.cpp
    src/PlatformWindows.cpp
    #src/PEFile.cpp
    src/ReadOnlyPacket.cpp
    src/RingBuffer.cpp
    src/ScriptEngine.cpp
    #src/Structgen.cpp
    src/TcpConnection.cpp
    src/TcpServer.cpp
    #src/ThreadManager.cpp
    src/Worker.cpp
    #src/XmlUtils.cpp
)

IF(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/ConfigCustom.cpp)
    SET(${PROJECT_NAME}_SRCS ${${PROJECT_NAME}_SRCS} src/ConfigCustom.cpp)
ELSE()
    SET(${PROJECT_NAME}_SRCS ${${PROJECT_NAME}_SRCS} src/Config.cpp)
ENDIF()

# This is a list of all header files. Adding the header files here ensures they
# are listed in the source files for IDE projects.
SET(${PROJECT_NAME}_HDRS
    src/Compress.h
    src/Constants.h
    src/Convert.h
    src/CString.h
    src/Database.h
    src/DatabaseCassandra.h
    src/DatabaseQuery.h
    src/DatabaseQueryCassandra.h
    src/DatabaseSQLite3.h
    src/Decrypt.h
    src/Endian.h
    src/EnumMap.h
    #src/EngineLocker.h
    src/Exception.h
    src/InternalConnection.h
    src/InternalServer.h
    src/LobbyConnection.h
    src/Log.h
    src/Manager.h
    #src/MemoryFile.h
    src/Message.h
    src/MessageEncrypted.h
    src/MessagePacket.h
    src/MessageQueue.h
    src/Object.h
    src/Packet.h
    src/PacketException.h
    #src/PacketScript.h
    #src/PEFile.h
    #src/PEFormat.h
    #src/Platform.h
    #src/PlatformLinux.h
    #src/PlatformWindows.h
    src/ReadOnlyPacket.h
    src/RingBuffer.h
    src/ScriptEngine.h
    #src/Structgen.h
    src/TcpConnection.h
    src/TcpServer.h
    #src/ThreadManager.h
    src/Worker.h
    #src/XmlUtils.h

    # These were generated and are not worth reading.
    src/LookupTableCP1252.h
    src/LookupTableCP932.h

    # Macros to temporarily disable warnings (wrapped around 3rd party libs).
    src/PushIgnore.h
    src/PopIgnore.h
)

SET(${PROJECT_NAME}_SCHEMA
    schema/test.xml
    schema/channelconfig.xml
    schema/lobbyconfig.xml
    schema/serverconfig.xml
    schema/worldconfig.xml
)

# Make an absolute path version of the schema list for the channel server to use as depends
FOREACH(schema ${${PROJECT_NAME}_SCHEMA})
    SET(LIBFROST_SCHEMA ${LIBFROST_SCHEMA} ${CMAKE_CURRENT_SOURCE_DIR}/${schema})
ENDFOREACH(schema ${${PROJECT_NAME}_SCHEMA})

# Make it available to the parent scope
SET(LIBFROST_SCHEMA ${LIBFROST_SCHEMA} PARENT_SCOPE)

OBJGEN_XML(${PROJECT_NAME}_STRUCTS
    # Main schema
    schema/master.xml

    # Included schemas
    ${${PROJECT_NAME}_SCHEMA}

    # Include paths
    schema

    # Output files
    ChannelConfig.h
    ChannelConfig.cpp
    LobbyConfig.h
    LobbyConfig.cpp
    ServerConfig.h
    ServerConfig.cpp
    WorldConfig.h
    WorldConfig.cpp
    TestObject.h
    TestObject.cpp
)

ADD_LIBRARY(comp STATIC ${${PROJECT_NAME}_SRCS} ${${PROJECT_NAME}_HDRS}
    ${${PROJECT_NAME}_MOC_SRCS} ${${PROJECT_NAME}_STRUCTS})

TARGET_LINK_LIBRARIES(comp ${OPENSSL_LIBRARIES} ${ZLIB_LIBRARIES}
    ${SYSTEM_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} sqlite3
    squirrel sqstdlib ttvfs ttvfs_zip tinyxml2 cassandra uv)

ADD_DEPENDENCIES(comp asio)

IF(WIN32)
    TARGET_LINK_LIBRARIES(comp advapi32 iphlpapi psapi shell32 userenv ws2_32)
ENDIF(WIN32)

# List of unit tests to add to CTest.
SET(${PROJECT_NAME}_TEST_SRCS
    Cassandra
    Convert
    Decrypt
    DiffieHellman
    GeneratedObjects
    Packet
    ScriptEngine
    String
    #XmlUtils
)

IF(NOT BSD)
    # Add the unit tests.
    CREATE_GTESTS(LIBS comp SRCS ${${PROJECT_NAME}_TEST_SRCS})
ENDIF(NOT BSD)
