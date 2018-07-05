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

CMAKE_MINIMUM_REQUIRED(VERSION 3.2.3)

#
# START OF VERSION CONSTANTS
#

# Last year the code was changed.
SET(VERSION_YEAR  2018)

# Major release (1 = pixie.)
SET(VERSION_MAJOR 3)

# Minor release (1 = SP1).
SET(VERSION_MINOR 1)

# Patch version (a hotfix).
SET(VERSION_PATCH 0)

# Codename for the version.
SET(VERSION_CODENAME "Lilim (SP1)")

#
# END OF VERSION CONSTANTS
#

# Make the code name friendly for the installer filename.
STRING(TOLOWER "${VERSION_CODENAME}" VERSION_CODENAME_C)
STRING(REPLACE " " "-" VERSION_CODENAME_C "${VERSION_CODENAME_C}")

# Use folders to organize the projects and files.
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

# Only show Debug and Release configurations in Visual Studio.
IF(CMAKE_CUSTOM_CONFIGURATION_TYPES)
    SET(CMAKE_CONFIGURATION_TYPES "${CMAKE_CUSTOM_CONFIGURATION_TYPES}" CACHE STRING "" FORCE)
ELSEIF(CMAKE_CONFIGURATION_TYPES)
    SET(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING "" FORCE)
ENDIF()

PROJECT(comp_hack)

# Print the CMake version for debugging.
MESSAGE("-- CMake version: ${CMAKE_VERSION}")

# Our custom cmake modules.
SET(COMP_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

# Include our custom cmake modules.
SET(CMAKE_MODULE_PATH
    ${COMP_MODULE_PATH}
    ${CMAKE_SOURCE_DIR}/coveralls-cmake/cmake
)

SET(CPACK_PACKAGE_NAME "COMP_hack Server (${VERSION_CODENAME})")
SET(CPACK_PACKAGE_VENDOR "COMP_hack Team")
SET(CPACK_PACKAGE_CONTACT "compomega@tutanota.com")
SET(CPACK_PACKAGE_FILE_NAME "comp_hack-${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}-${VERSION_CODENAME_C}")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.txt")
SET(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH})

IF(WIN32)
    SET(COMP_INSTALL_DIR "/")
    SET(CPACK_GENERATOR "ZIP;WIX")

    #SET(CPACK_WIX_ROOT "C:\\Program Files (x86)\\WiX Toolset v3.11")
    SET(CPACK_WIX_UPGRADE_GUID "A253D8BE-ED6C-43EF-A1C5-F378F269C9E4")
    SET(CPACK_WIX_PRODUCT_ICON "${CMAKE_SOURCE_DIR}/tools/bdpatch/res/bdpatch.ico")
    SET(CPACK_WIX_PROGRAM_MENU_FOLDER "COMP_hack")
    SET(CPACK_WIX_PROPERTY_ARPHELPLINK "https://github.com/comphack/comp_hack")
    SET(CPACK_WIX_PATCH_FILE "${CMAKE_SOURCE_DIR}/contrib/services.wxs")
    SET(CPACK_PACKAGE_INSTALL_DIRECTORY "COMP_hack")
    SET(CPACK_PACKAGE_EXECUTABLES "comp_capgrep;Capture Analysis;comp_logger;Capture Logger;comp_map;Enemy Mapping")

    SET(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)

    INCLUDE(InstallRequiredSystemLibraries)

    INSTALL(
        PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
        DESTINATION /
    )

    FIND_PACKAGE(Qt5Widgets REQUIRED)

    IF(Qt5Widgets_FOUND AND WIN32 AND TARGET Qt5::qmake AND NOT TARGET Qt5::windeployqt)
        GET_TARGET_PROPERTY(_qt5_qmake_location Qt5::qmake IMPORTED_LOCATION)

        EXECUTE_PROCESS(
            COMMAND "${_qt5_qmake_location}" -query QT_INSTALL_PREFIX
            RESULT_VARIABLE return_code
            OUTPUT_VARIABLE qt5_install_prefix
            OUTPUT_STRIP_TRAILING_WHITESPACE)

        SET(imported_location "${qt5_install_prefix}/bin/windeployqt.exe")

        IF(EXISTS ${imported_location})
            ADD_EXECUTABLE(Qt5::windeployqt IMPORTED)

            SET_TARGET_PROPERTIES(Qt5::windeployqt PROPERTIES
                IMPORTED_LOCATION ${imported_location})
        ENDIF()
    ENDIF()

    INSTALL(FILES
        "${CMAKE_SOURCE_DIR}/contrib/winconfig/lobby.xml"
        "${CMAKE_SOURCE_DIR}/contrib/winconfig/channel.xml"
        "${CMAKE_SOURCE_DIR}/contrib/winconfig/world.xml"
        "${CMAKE_SOURCE_DIR}/contrib/winconfig/setup.xml"
        "${CMAKE_SOURCE_DIR}/contrib/winconfig/newcharacter.xml"
        "${CMAKE_SOURCE_DIR}/contrib/testing/constants.xml"
        DESTINATION config)
ELSE()
    SET(COMP_INSTALL_DIR "bin")
    SET(CPACK_GENERATOR "TGZ;TBZ2;DEB")
ENDIF()

IF(NOT WIN32)
    INCLUDE(GNUInstallDirs)
ENDIF(NOT WIN32)

# Enable the CPack module for building installers.
INCLUDE(CPack)

# Utilities to add and remove compiler flags.
INCLUDE(${COMP_MODULE_PATH}/flags.cmake)

# Utilities for building with MSVC.
INCLUDE(${COMP_MODULE_PATH}/msvc.cmake)

# Option to combine packets into one source to build.
OPTION(SINGLE_SOURCE_PACKETS "Combine all packets into one source file." OFF)

# Option to compile all objgen objects into one source file.
OPTION(SINGLE_OBJGEN "Compile all objgen objects into one source file." OFF)

# Use Cotire to generate pre-compiled headers.
OPTION(USE_COTIRE "Use cotire to build with pre-compiled headers." OFF)

IF(USE_COTIRE)
    INCLUDE(cotire)
ENDIF(USE_COTIRE)

# Option to disable build warnings/errors.
OPTION(NO_WARNINGS "Disable the compiler warnings and errors." OFF)

# Option to disable all tests.
OPTION(DISABLE_TESTING "Disable all tests." OFF)

# Option for the static runtime on Windows.
OPTION(USE_STATIC_RUNTIME "Use the static MSVC runtime." OFF)

IF(WIN32)
    OPTION(GENERATE_DOCUMENTATION "Generate documentation for the project." OFF)
ELSE()
    OPTION(GENERATE_DOCUMENTATION "Generate documentation for the project." ON)
ENDIF()

# Make sure MSVC uses the right runtime.
IF(USE_STATIC_RUNTIME)
    MSVC_RUNTIME(STATIC)
ELSE()
    MSVC_RUNTIME(DYNAMIC)
ENDIF()

IF(WIN32)
    OPTION(WINDOWS_SERVICE "Build the servers as a Windows service." OFF)

    IF(WINDOWS_SERVICE)
        ADD_DEFINITIONS(-DWIN32_SERV=1)
    ENDIF()
ENDIF()

# http://stackoverflow.com/questions/14933172/
IF("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # Require at least GCC 4.9.
    IF(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
        MESSAGE(FATAL_ERROR "GCC version must be at least 4.9!")
    ENDIF()

    ADD_COMPILER_FLAGS(AUTO -fno-strict-aliasing)
ELSEIF("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    SET(SPECIAL_COMPILER_FLAGS "-stdlib=libc++")

    # Require at least Clang 3.4.
    IF(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.4)
        MESSAGE(FATAL_ERROR "Clang version must be at least 3.4!")
    ENDIF()
ELSEIF("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    # There is nothing special needed for MSVC.
ELSE()
    MESSAGE(WARNING "You are using an unsupported compiler!")
ENDIF()

INCLUDE(${COMP_MODULE_PATH}/DetermineOS.cmake)

# If code coverage should be generated.
OPTION(COVERALLS "Generate coveralls code coverage data." OFF)

IF(COVERALLS)
    INCLUDE(Coveralls)
ENDIF(COVERALLS)

# If the build should be optimized.
OPTION(BUILD_OPTIMIZED "Build an optimized release of the server." ON)

IF(BUILD_OPTIMIZED)
    SET(BUILD_VALGRIND_FRIENDLY OFF)
ELSE()
    SET(BUILD_VALGRIND_FRIENDLY ON)
ENDIF()

# Include all build code for external projects.
IF(EXISTS "${CMAKE_SOURCE_DIR}/binaries")
    MESSAGE("-- Using pre-built binaries.")

    INCLUDE(${COMP_MODULE_PATH}/binaries.cmake)
ELSE()
    MESSAGE("-- Building external binaries.")

    INCLUDE(${COMP_MODULE_PATH}/external.cmake)
ENDIF()

# PDB not found for library.
ADD_LINKER_FLAGS(AUTO /ignore:4099)

# UTF-8 source encoding for MSVC.
ADD_COMPILER_FLAGS(AUTO /utf-8)

# Build for Windows 7 and higher.
# See: https://msdn.microsoft.com/en-us/library/6sehtctf.aspx
ADD_COMPILER_FLAGS(WIN32 -DWINVER=0x601 -D_WIN32_WINNT=0x0601)

# Don't use full boost, just asio.
ADD_DEFINITIONS(-DASIO_STANDALONE)

# Include more Sqrat types from libcomp.
ADD_DEFINITIONS(
    -DSQRAT_EXTRA_TYPES_INCLUDE=<SqratTypesInclude.h>
    -DSQRAT_EXTRA_TYPES_SOURCE=<SqratTypesSource.h>
    -DSQRAT_EXTRA_TYPES_NONREF=<SqratTypesNonRef.h>
)

# SQLite3 is included in the source repo.
SET(SQLITE3_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/deps/sqlite3")

# Sqrat is included as a submodule (it's header only).
SET(SQRAT_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/deps/sqrat/include")
SET(SQRAT_DEFINES "-DSCRAT_USE_CXX11_OPTIMIZATIONS=1")

# Enable testing.
IF(NOT DISABLE_TESTING)
    INCLUDE(CTest)
ENDIF(NOT DISABLE_TESTING)

# Determine if the system is big or little endian.
INCLUDE(TestBigEndian)
TEST_BIG_ENDIAN(LIBCOMP_ENDIAN)
IF(${LIBCOMP_ENDIAN})
    ADD_DEFINITIONS(-DLIBCOMP_BIGENDIAN)
ELSE(${LIBCOMP_ENDIAN})
    ADD_DEFINITIONS(-DLIBCOMP_LITTLEENDIAN)
ENDIF(${LIBCOMP_ENDIAN})

# Default Linux (gcc/clang) builds to debug and MinGW builds to release.
IF(NOT MSVC)
    IF(BUILD_OPTIMIZED)
        SET(CMAKE_BUILD_TYPE Release)
    ELSE()
        SET(CMAKE_BUILD_TYPE Debug)
    ENDIF()

    # Ensure C++14 support is on.
    ADD_CXX_FLAGS(AUTO -std=c++14)

    IF(NOT ("${SPECIAL_COMPILER_FLAGS}" STREQUAL ""))
        ADD_CXX_FLAGS(AUTO "${SPECIAL_COMPILER_FLAGS}")
    ENDIF()

    # Determine basic gcc/clang/mingw flags for release mode.
    IF(BUILD_OPTIMIZED)
        ADD_COMPILER_FLAGS(AUTO -O3 -msse3)
    ELSE(BUILD_OPTIMIZED)
        ADD_COMPILER_FLAGS(AUTO -O3 -msse3)
    ENDIF(BUILD_OPTIMIZED)

    ADD_LINKER_FLAGS_RELEASE(AUTO -s)
ENDIF(NOT MSVC)

# Disable MSVC warnings about the secure CRT functions.
ADD_COMPILER_FLAGS(WIN32 -D_CRT_SECURE_NO_WARNINGS)

# Specifies the kind of exception handling with MSVC.
ADD_COMPILER_FLAGS(AUTO /EHsc)

# If we are building in debug mode, define the debug flag.
ADD_COMPILER_FLAGS_DEBUG(AUTO -DCOMP_HACK_DEBUG)

# If we are using gcc/clang/mingw, enable warnings under debug mode.
IF(NO_WARNINGS)
    ADD_COMPILER_FLAGS(AUTO -g)
ELSE()
    ADD_COMPILER_FLAGS(AUTO -Werror -Wall -Wextra -Wshadow
        -Wconversion -Wsign-conversion -g)
ENDIF()

# Warning level 4 and treat warnings as errors (MSVC).
# ADD_COMPILER_FLAGS(AUTO /W4 /WX)

# When using gcc/clang/mingw, make sure everything defined is linked into
# the application or library.
ADD_EXE_LINKER_FLAGS(AUTO -Wl,--no-undefined)

# Code coverage flags.
IF(COVERALLS)
    ADD_COMPILER_FLAGS(AUTO -g -O0 -fprofile-arcs -ftest-coverage)
    REMOVE_COMPILER_FLAGS(-Os -O2 -O3)
ENDIF(COVERALLS)

# Place all executables in the same directory.
SET(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}/bin")

# List of common include paths for every project using libobjgen.
SET(LIBOBJGEN_INCLUDES
    ${GSL_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/libobjgen/src
    # Needed for PushIgnore.h and PopIgnore.h.
    ${CMAKE_SOURCE_DIR}/libcomp/src
)

# List of common include paths for every project using libcomp.
SET(LIBCOMP_INCLUDES
    ${OPENSSL_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/libcomp/src
    ${CMAKE_BINARY_DIR}/libcomp/objgen
)

SET(COVERALLS_SRCS "" CACHE INTERNAL "Coverage source files")

# Add sources to the coverage source list.
MACRO(COVERALLS_SOURCES)
    FOREACH(it ${ARGN})
        FILE(RELATIVE_PATH filePath "${CMAKE_SOURCE_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/${it}")

        SET(COVERALLS_SRCS ${COVERALLS_SRCS} "${filePath}"
            CACHE INTERNAL "Coverage source files")
    ENDFOREACH()
ENDMACRO()

MACRO(ADD_QT_DEPS target)
    # See: https://stackoverflow.com/questions/41193584/deploy-all-qt-dependencies-when-building
    IF(TARGET Qt5::windeployqt)
        ADD_CUSTOM_COMMAND(TARGET ${target}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_CURRENT_BINARY_DIR}/windeployqt"
            COMMAND set PATH=%PATH%$<SEMICOLON>${qt5_install_prefix}/bin
            COMMAND Qt5::windeployqt --dir "${CMAKE_CURRENT_BINARY_DIR}/windeployqt" "$<TARGET_FILE_DIR:${target}>/$<TARGET_FILE_NAME:${target}>"
        )

        INSTALL(
            DIRECTORY
            "${CMAKE_CURRENT_BINARY_DIR}/windeployqt/"
            DESTINATION /
        )
    ENDIF()
ENDMACRO()

# This macro will create a target to generate the documentation using the
# specified Doxyfile.in file.
MACRO(GENERATE_DOCS doxyfile)
    # Make sure we have Doxygen.
    FIND_PACKAGE(Doxygen)

    # Only do something if we have Doxygen.
    If(DOXYGEN_FOUND AND GENERATE_DOCUMENTATION)
        # Replace CMake variables in the input Doxyfile.
        CONFIGURE_FILE(${doxyfile} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)

        # Create the target that will run Doxygen. The working directory is
        # the build directory so all the documentation ends up in the same
        # directory structure.
        IF(${PROJECT_NAME} MATCHES "libcomp")
            ADD_CUSTOM_TARGET(doc-${PROJECT_NAME} ${DOXYGEN_EXECUTABLE}
                ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile WORKING_DIRECTORY
                ${CMAKE_BINARY_DIR} COMMENT
                "Generating ${PROJECT_NAME} API documentation" VERBATIM)
        ELSE(${PROJECT_NAME} MATCHES "libcomp")
            ADD_CUSTOM_TARGET(doc-${PROJECT_NAME} ${DOXYGEN_EXECUTABLE}
                ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile WORKING_DIRECTORY
                ${CMAKE_BINARY_DIR} DEPENDS doc-libcomp COMMENT
                "Generating ${PROJECT_NAME} API documentation" VERBATIM)
        ENDIF(${PROJECT_NAME} MATCHES "libcomp")

        # Add the target to a list of documentation targets.
        GET_PROPERTY(targets GLOBAL PROPERTY DOC_TARGETS)
        SET_PROPERTY(GLOBAL PROPERTY DOC_TARGETS
            doc-${PROJECT_NAME} ${targets})
    ENDIF(DOXYGEN_FOUND AND GENERATE_DOCUMENTATION)
ENDMACRO(GENERATE_DOCS doxyfile)

# When building Windows executables, this macro packs the executable. This
# works by simply passing the same name you passed to ADD_EXECUTABLE to this
# macro. The executable will be packed in place. UPX is not used when using
# Microsoft Visual C++ or if DISABLE_UPX is defined.
MACRO(UPX_WRAP exefile)
IF(WIN32 AND NOT MSVC AND NOT DISABLE_UPX)
    # Get the path to the executable.
    GET_PROPERTY(exefile_path TARGET ${exefile} PROPERTY LOCATION)

    # Add a command to run UPX passing a compression of 9 and the path to
    # the target executable.
    ADD_CUSTOM_COMMAND(TARGET ${exefile} POST_BUILD
        COMMAND upx -9 ${exefile_path} 1> nul 2>&1)
ENDIF(WIN32 AND NOT MSVC AND NOT DISABLE_UPX)
ENDMACRO(UPX_WRAP exefile)

# If we are using mingw and the path to windres is not set, add a default path.
IF(MINGW AND NOT CMAKE_WINDRES_PATH)
    SET(CMAKE_WINDRES_PATH windres.exe)
ENDIF(MINGW AND NOT CMAKE_WINDRES_PATH)

# This macro is used to compile Windows resource files for either Microsoft
# Visual C++ or MinGW. Simply pass the name of the output variable followed
# by a list of resource file paths. The output variable will be filled and
# should then be passed as source files to the ADD_EXECUTABLE command.
MACRO(RES_WRAP outfiles)
IF(WIN32)
    IF(MINGW) # MinGW
        FOREACH(it ${ARGN}) # Process each resource file
            # Get the name of the file (without the extension) and the path
            # to the file. These are needed for the custom command.
            GET_FILENAME_COMPONENT(fn ${it} NAME_WE)
            GET_FILENAME_COMPONENT(fp ${it} PATH)

            # This command calls windres with the resource file and outputs
            # an object file with the _res.o suffix. This object file is then
            # linked to the executable (by adding the object file to the output
            # variable). The object file depends on the resource file.
            ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${fn}_res.o
                COMMAND ${CMAKE_WINDRES_PATH}
                -I${CMAKE_CURRENT_SOURCE_DIR}/${fp}
                -i${CMAKE_CURRENT_SOURCE_DIR}/${it}
                -o ${CMAKE_CURRENT_BINARY_DIR}/${fn}_res.o
                DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${it})

            # Add the object file to the list of output files that will be
            # added to the ADD_EXECUTABLE command (and thus linked to the app).
            SET(${outfiles} ${${outfiles}}
                ${CMAKE_CURRENT_BINARY_DIR}/${fn}_res.o)
        ENDFOREACH(it ${ARGN})
    ELSE(MINGW) # Microsoft Visual C++
        FOREACH(it ${ARGN}) # Process each resource file
            # Simply add the resource file to the output variable and let cmake
            # handle it for us.
            SET(${outfiles} ${${outfiles}}
                ${CMAKE_CURRENT_SOURCE_DIR}/${it})
        ENDFOREACH(it ${ARGN})
    ENDIF(MINGW)
ENDIF(WIN32)
ENDMACRO(RES_WRAP outfiles)

# This macro handles Qt translations and embeds them as resources into the
# application under the "/trans" virtual directory. The arguments must start
# with the name of the output variable that will be passed to the
# ADD_EXECUTABLE command to ensure the generated files are compiled and linked
# to the application. All remaining variables are source files containing text
# to be translated and source translation files to be built into the app.
MACRO(SETUP_TRANSLATION outvar)
    # Generate a list of translation source files.
    SET(ts_files "")
    FOREACH(it ${ARGN})
        GET_FILENAME_COMPONENT(ex ${it} EXT)

        IF(ex MATCHES "ts")
            SET(ts_files ${ts_files} ${it})
        ENDIF(ex MATCHES "ts")
    ENDFOREACH(it ${ARGN})

    # If the user has instructed us to generate the translation source files,
    # do that; otherwise, add them to be compiled into binary files.
    IF(${CREATE_TRANSLATION})
        QT4_CREATE_TRANSLATION(qm_out ${ARGN})
    ELSE(${CREATE_TRANSLATION})
        QT4_ADD_TRANSLATION(qm_out ${ts_files})
    ENDIF(${CREATE_TRANSLATION})

    # Generate the contents of the translation resource file.
    SET(qrc_contents "<!DOCTYPE RCC><RCC version=\"1.0\">")
    SET(qrc_contents "${qrc_contents}<qresource prefix=\"/trans\">")
    FOREACH(it ${ts_files})
        GET_FILENAME_COMPONENT(fn ${it} NAME_WE)

        SET(qrc_contents "${qrc_contents}<file>${fn}.qm</file>")
    ENDFOREACH(it ${ARGN})
    SET(qrc_contents "${qrc_contents}</qresource></RCC>")

    # Where to write the translation resource file to.
    SET(qrc_path ${CMAKE_CURRENT_BINARY_DIR}/trans.qrc)

    # Write the translation resource file.
    FILE(WRITE ${qrc_path} "${qrc_contents}")

    # Add the translation resource file as a target to be generated.
    QT4_ADD_RESOURCES(qrc_src ${qrc_path})

    # Set the output variable as the generated resource target.
    SET(${outvar} ${qrc_src})
ENDMACRO(SETUP_TRANSLATION outvar)

# This macro takes a list of test names (for google-test, not cucumber), builds
# them, and adds them to the CTest framework. Note that the test file should
# be in the "tests" subdirectory of the project with the test name and a ".cpp"
# extension for this macro to work.
MACRO(CREATE_GTESTS)
    SET(EXPECT_TARGET False)
    SET(TEST_TARGET "test")
    SET(TEST_LIBS "")
    SET(HAVE_LIBS False)

    # Create a test based on each test name.
    FOREACH(test ${ARGN})
        IF("${test}" MATCHES "LIBS")
            SET(HAVE_LIBS False)
        ELSEIF("${test}" MATCHES "SRCS")
            SET(HAVE_LIBS True)
        ELSEIF("${test}" MATCHES "TARGET")
            SET(EXPECT_TARGET True)
        ELSEIF(EXPECT_TARGET)
            SET(TEST_TARGET ${test})
            SET(EXPECT_TARGET False)
        ELSEIF(HAVE_LIBS)
            # Prefix the test name with "Test".
            SET(ttest "Test${test}")

            # Generate the test executable.
            ADD_EXECUTABLE(${ttest} "tests/${test}.cpp")

            # Add this to the project folder.
            SET_TARGET_PROPERTIES(${ttest} PROPERTIES FOLDER
                "Tests/${PROJECT_NAME}")

            # Link the libraries to the test executable.
            TARGET_LINK_LIBRARIES(${ttest} gtest ${TEST_LIBS}
                ${CMAKE_THREAD_LIBS_INIT} ${ZLIB_LIBRARIES})

            # Add the test to CTest.
            IF("${TEST_TARGET}" STREQUAL "test")
                ADD_TEST(NAME ${test} COMMAND
                    ${EXECUTABLE_OUTPUT_PATH}/${ttest}
                    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
            ELSE()
                ADD_CUSTOM_COMMAND(TARGET ${TEST_TARGET} COMMAND
                    ${EXECUTABLE_OUTPUT_PATH}/${ttest}
                    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                    DEPENDS ${ttest})
            ENDIF()
        ELSE() # Must be a library.
            # Add the library to the list.
            SET(TEST_LIBS ${TEST_LIBS} ${test})
        ENDIF("${test}" MATCHES "LIBS")
    ENDFOREACH(test ${ARGN})
ENDMACRO(CREATE_GTESTS)

# This must come first so that objgen is found for the macro bellow. As a
# consequence, none of the tools can define their own structures to be
# generated by objgen. This is not a big deal because for the most part
# these should be defined in libcomp.
ADD_SUBDIRECTORY(deps)
ADD_SUBDIRECTORY(libobjgen)
ADD_SUBDIRECTORY(tools)

# This macro generates code using objgen. The arguments must start with the
# name of the output variable that will be passed to the ADD_EXECUTABLE command
# to ensure the generated files are compiled and linked to the application or
# library. The 2nd argument must be the main xml schema file that includes all
# other schema files and structures that code will be generated for. The
# remaining arguments will change depending on the extension (or lack of one).
# Files with the xml extension will be used as dependencies to the master xml
# schema. These are xml schema files that have been declared in an <include>
# element. Files that end in cpp or h are source files that will be generated.
# Only the source files defined will be generated despite what structures may
# be included in the xml schema. Finally, all other arguments are assumed to be
# a search path for other xml schema files that have be listed in an <include>
# element.
MACRO(OBJGEN_XML outfiles xml)
    # Get the absolute path to the master xml schema.
    GET_FILENAME_COMPONENT(xml_abs ${xml} ABSOLUTE)

    # Set the master xml schema as a dependency.
    SET(deps ${xml_abs})

    IF(SINGLE_OBJGEN)
        FILE(WRITE "${CMAKE_CURRENT_BINARY_DIR}/objgen/ObjectCollection.cpp.in"
            "// This file is generated by CMake! DO NOT EDIT!\n")
    ENDIF(SINGLE_OBJGEN)

    # For each argument after the output variable and master xml schema.
    FOREACH(it ${ARGN})
        # Get the absolute path and extension to the file or directory.
        GET_FILENAME_COMPONENT(fp ${it} ABSOLUTE)
        GET_FILENAME_COMPONENT(ex ${it} EXT)

        IF(ex MATCHES "xml") # XML schema file
            # Add the xml schema file as a dependency.
            SET(deps ${deps} "${fp}")
        ELSEIF(ex MATCHES "cpp" OR ex MATCHES "h") # Source or header file
            # Add the source or header file as a generated output that must
            # then be compiled.
            SET(outs ${outs} "${CMAKE_CURRENT_BINARY_DIR}/objgen/${it}")

            # Add the source as a file to include for the single source file.
            IF(SINGLE_OBJGEN AND ex MATCHES "cpp")
                FILE(APPEND
                    "${CMAKE_CURRENT_BINARY_DIR}/objgen/ObjectCollection.cpp.in"
                    "#include \"${it}\"\n")
            ENDIF(SINGLE_OBJGEN AND ex MATCHES "cpp")
        ELSE() # Everything else is assumed to be a directory
            # Add the directory as a search path for other xml schema files.
            SET(incs ${incs} "-I" "${fp}")
        ENDIF(ex MATCHES "xml")
    ENDFOREACH(it ${ARGN})

    # Add custom commands for all output source or header files so that they
    # depend on all xml schema files listed and are generated when those files
    # or the objgen application change.
    FOREACH(out ${outs})
        ADD_CUSTOM_COMMAND(OUTPUT ${out}
            COMMAND comp_objgen ${incs} -o ${out} ${xml_abs}
            COMMAND cmake -E touch ${out}
            DEPENDS comp_objgen ${deps})
    ENDFOREACH(out ${outs})

    # Set the list of output files to be generated, compiled, and linked.
    IF(SINGLE_OBJGEN)
        ADD_CUSTOM_COMMAND(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/objgen/ObjectCollection.cpp"
            COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_BINARY_DIR}/objgen/ObjectCollection.cpp.in"
            "${CMAKE_CURRENT_BINARY_DIR}/objgen/ObjectCollection.cpp"
            DEPENDS ${outs})

        SET(${outfiles} ${${outfiles}} "${CMAKE_CURRENT_BINARY_DIR}/objgen/ObjectCollection.cpp")
    ELSE(SINGLE_OBJGEN)
        SET(${outfiles} ${${outfiles}} ${outs})
    ENDIF(SINGLE_OBJGEN)
ENDMACRO(OBJGEN_XML outfiles xml)

# Macro to replace the packet source files with a single source file to build.
MACRO(COMBINE_PACKETS listvar)
FILE(WRITE "${CMAKE_CURRENT_BINARY_DIR}/Packets.cpp" "")

    FOREACH(src IN LISTS ${listvar})
        STRING(REGEX REPLACE "^src/(.+)$" "#include \"\\1\"\n" src "${src}")
        FILE(APPEND "${CMAKE_CURRENT_BINARY_DIR}/Packets.cpp" "${src}")
    ENDFOREACH()

    SET(${listvar} "${CMAKE_CURRENT_BINARY_DIR}/Packets.cpp")
ENDMACRO(COMBINE_PACKETS listvar)

# Macro to create rspec tests (web testing with ruby).
MACRO(RSPEC_TESTS)
    FOREACH(test ${ARGN})
        ADD_TEST(NAME ${test} COMMAND rspec ${test}.rb
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/rspec)
        SET_PROPERTY(TEST ${test} PROPERTY ENVIRONMENT
            "TESTING_DIR=${CMAKE_BINARY_DIR}")
    ENDFOREACH(test ${ARGN})
ENDMACRO(RSPEC_TESTS)

IF(NOT WIN32 AND NOT BSD)
    FIND_PACKAGE(Systemd QUIET)

    IF(SYSTEMD_FOUND)
        ADD_COMPILER_FLAGS(AUTO ${SYSTEMD_DEFINITIONS} -DHAVE_SYSTEMD=1)
    ENDIF(SYSTEMD_FOUND)
ENDIF(NOT WIN32 AND NOT BSD)

ADD_SUBDIRECTORY(libcomp)
#ADD_SUBDIRECTORY(chanman)

# If the client code exists in the "client" directory and DISABLE_CLIENT is not
# defined, build the client application as well.
IF(IS_DIRECTORY "${CMAKE_SOURCE_DIR}/client" AND NOT ${DISABLE_CLIENT})
    #ADD_SUBDIRECTORY(client)
ENDIF(IS_DIRECTORY "${CMAKE_SOURCE_DIR}/client" AND NOT ${DISABLE_CLIENT})

#ADD_SUBDIRECTORY(console)
ADD_SUBDIRECTORY(server)

# Windows does not have the server spawning code.
IF(NOT WIN32)
    ADD_SUBDIRECTORY(libtester)
    ADD_SUBDIRECTORY(client)
ENDIF(NOT WIN32)

#ADD_SUBDIRECTORY(updater)

# Add all the documentation targets to a single target "doc".
GET_PROPERTY(targets GLOBAL PROPERTY DOC_TARGETS)
ADD_CUSTOM_TARGET(doc ALL DEPENDS ${targets})
SET_TARGET_PROPERTIES(doc PROPERTIES FOLDER "SpecialTargets")
ADD_SUBDIRECTORY(docs)

IF(NOT BSD)
    #ADD_SUBDIRECTORY(features)
ENDIF(NOT BSD)

# On Linux system, install the icons where they can be found. These are used
# for the application menu launcher icons.
IF(NOT WIN32)
    INSTALL(FILES icons/16x16/comp_hack.png
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/16x16/apps)
    INSTALL(FILES icons/22x22/comp_hack.png
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/22x22/apps)
    INSTALL(FILES icons/24x24/comp_hack.png
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/24x24/apps)
    INSTALL(FILES icons/32x32/comp_hack.png
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/32x32/apps)
    INSTALL(FILES icons/48x48/comp_hack.png
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/48x48/apps)
    INSTALL(FILES icons/64x64/comp_hack.png
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/64x64/apps)
    INSTALL(FILES icons/64x64/comp_hack.png
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/app-install/icons)
    INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/contrib/systemd/
        DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/systemd/system)
    INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/contrib/webroot
        DESTINATION ${CMAKE_INSTALL_LOCALSTATEDIR}/lib/comp_hack)
    INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/contrib/nixconfig/
        DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/comp_hack)
    INSTALL(FILES ${CMAKE_SOURCE_DIR}/contrib/testing/constants.xml
        DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/comp_hack)
    INSTALL(FILES ${CMAKE_SOURCE_DIR}/contrib/winconfig/newcharacter.xml
        DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/comp_hack)
ENDIF(NOT WIN32)

# Copy all contrib/testing to the bin directory.
FILE(COPY "${CMAKE_SOURCE_DIR}/contrib/testing"
    DESTINATION "${CMAKE_BINARY_DIR}/bin")

IF(COVERALLS)
    coveralls_setup("${COVERALLS_SRCS}" ON)
ENDIF(COVERALLS)

# Make sure we have Doxygen.
FIND_PACKAGE(Doxygen)

If(DOXYGEN_FOUND AND GENERATE_DOCUMENTATION AND NOT WIN32)
    INSTALL(DIRECTORY ${CMAKE_BINARY_DIR}/api
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/doc/comp_hack)
ENDIF(DOXYGEN_FOUND AND GENERATE_DOCUMENTATION AND NOT WIN32)
