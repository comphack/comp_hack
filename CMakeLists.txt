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

PROJECT(comp_hack)

# http://stackoverflow.com/questions/14933172/
IF("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # Require at least GCC 4.9.
    IF(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
        MESSAGE(FATAL_ERROR "GCC version must be at least 4.9!")
    ENDIF()
ELSEIF("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    SET(SPECIAL_COMPILER_FLAGS "-stdlib=libc++")

    # Require at least Clang 3.4.
    IF(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.4)
        MESSAGE(FATAL_ERROR "Clang version must be at least 3.4!")
    ENDIF()
ELSEIF("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd -DASIO_STANDALONE -D_WIN32_WINNT=0x0502 /utf-8")
    SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd -DASIO_STANDALONE -D_WIN32_WINNT=0x0502 /utf-8")

    SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT -DASIO_STANDALONE -D_WIN32_WINNT=0x0502 /utf-8")
    SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT -DASIO_STANDALONE -D_WIN32_WINNT=0x0502 /utf-8")

    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ignore:4099")
ELSE()
    MESSAGE(WARNING "You are using an unsupported compiler!")
ENDIF()

# Print the CMake version for debugging.
MESSAGE("-- CMake version: ${CMAKE_VERSION}")

# Include our custom cmake modules.
SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

INCLUDE(${CMAKE_MODULE_PATH}/DetermineOS.cmake)

# Include all build code for external projects.
INCLUDE(${CMAKE_MODULE_PATH}/external.cmake)

# SQLite3 is included in the source repo.
SET(SQLITE3_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/deps/sqlite3")

# Enable testing.
INCLUDE(CTest)

# Default Linux (gcc/clang) builds to debug and MinGW builds to release.
IF(NOT MSVC)
IF(WIN32)
    SET(CMAKE_BUILD_TYPE Release)
ELSE(WIN32)
    SET(CMAKE_BUILD_TYPE Debug)
ENDIF(WIN32)

# If the build should be optimized for new processors.
SET(BUILD_SERVER FALSE)

# Determine if the system is big or little endian.
INCLUDE(TestBigEndian)
TEST_BIG_ENDIAN(LIBCOMP_ENDIAN)
IF(${LIBCOMP_ENDIAN})
    ADD_DEFINITIONS("-DLIBCOMP_BIGENDIAN")
ELSE(${LIBCOMP_ENDIAN})
    ADD_DEFINITIONS("-DLIBCOMP_LITTLEENDIAN")
ENDIF(${LIBCOMP_ENDIAN})

ADD_DEFINITIONS(-DASIO_STANDALONE)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 ${SPECIAL_COMPILER_FLAGS}")

# Determine basic gcc/clang/mingw flags for release mode.
IF(BUILD_SERVER)
    SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -msse3 -s")
    SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -msse3 -s")
ELSE(BUILD_SERVER)
    SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O2 -msse2 -s")
    SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -msse2 -s")
ENDIF(BUILD_SERVER)
ENDIF(NOT MSVC)

# If building on Microsoft Visual C++, we need these flags.
IF(MSVC)
SET(CMAKE_C_FLAGS "-D_CRT_SECURE_NO_WARNINGS")
SET(CMAKE_CXX_FLAGS "-D_CRT_SECURE_NO_WARNINGS /EHsc")
ENDIF(MSVC)

# If we are building in debug mode, define the debug flag.
SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DCOMP_HACK_DEBUG")
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DCOMP_HACK_DEBUG")

# If we are using gcc/clang/mingw, enable warnings under debug mode.
IF(NOT MSVC)
SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -Wextra -Wshadow -g")
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Wshadow -g")

# If we are using clang (very similar to gcc), disable this warning.
IF(BUILD_CLANG)
    SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wno-sign-conversion")
ELSE(BUILD_CLANG)
    SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wconversion")
ENDIF(BUILD_CLANG)

SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Werror")

IF(NOT BSD)
SET(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_FLAGS_DEBUG} -Wno-delete-non-virtual-dtor")
ENDIF(NOT BSD)
ENDIF(NOT MSVC)

# When using gcc/clang/mingw, make sure everything defined is linked into
# the application or library.
IF(NOT MSVC)
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-undefined")
ENDIF(NOT MSVC)

# Place all executables in the same directory.
SET(EXECUTABLE_OUTPUT_PATH "${CMAKE_CURRENT_BINARY_DIR}/bin")

# List of common include paths for every project using libobjgen.
SET(LIBOBJGEN_INCLUDES
    ${CMAKE_SOURCE_DIR}/libobjgen/src
    # Needed for PushIgnore.h and PopIgnore.h.
    ${CMAKE_SOURCE_DIR}/libcomp/src
)

# List of common include paths for every project using libcomp.
SET(LIBCOMP_INCLUDES
    ${OPENSSL_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/eigen2
    ${CMAKE_SOURCE_DIR}/libcomp/src
    ${CMAKE_BINARY_DIR}/libcomp/objgen
)

# This macro will create a target to generate the documentation using the
# specified Doxyfile.in file.
MACRO(GENERATE_DOCS doxyfile)
    # Make sure we have Doxygen.
    FIND_PACKAGE(Doxygen)

    # Only do something if we have Doxygen.
    If(DOXYGEN_FOUND)
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
    ENDIF(DOXYGEN_FOUND)
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
    SET(TEST_LIBS "")
    SET(HAVE_LIBS False)

    # Create a test based on each test name.
    FOREACH(test ${ARGN})
        IF(${test} MATCHES "LIBS")
            SET(HAVE_LIBS False)
        ELSEIF(${test} MATCHES "SRCS")
            SET(HAVE_LIBS True)
        ELSEIF(HAVE_LIBS)
            # Prefix the test name with "Test".
            SET(ttest "Test${test}")

            # Generate the test executable.
            ADD_EXECUTABLE(${ttest} "tests/${test}.cpp")

            # Link the libraries to the test executable.
            TARGET_LINK_LIBRARIES(${ttest} gtest ${TEST_LIBS}
                ${CMAKE_THREAD_LIBS_INIT} ${ZLIB_LIBRARIES})

            # Add the test to CTest.
            ADD_TEST(NAME ${test} COMMAND ${EXECUTABLE_OUTPUT_PATH}/${ttest})
        ELSE() # Must be a library.
            # Add the library to the list.
            SET(TEST_LIBS ${TEST_LIBS} ${test})
        ENDIF(${test} MATCHES "LIBS")
    ENDFOREACH(test ${ARGN})
ENDMACRO(CREATE_GTESTS)

# This must come first so that objgen is found for the macro bellow. As a
# consequence, none of the tools can define their own structures to be
# generated by objgen. This is not a big deal because for the most part
# these should be defined in libcomp.
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
    SET(${outfiles} ${${outfiles}} ${outs})
ENDMACRO(OBJGEN_XML outfiles xml)

ADD_SUBDIRECTORY(libcomp)
#ADD_SUBDIRECTORY(chanman)

# If the client code exists in the "client" directory and DISABLE_CLIENT is not
# defined, build the client application as well.
IF(IS_DIRECTORY "${CMAKE_SOURCE_DIR}/client" AND NOT ${DISABLE_CLIENT})
    #ADD_SUBDIRECTORY(client)
ENDIF(IS_DIRECTORY "${CMAKE_SOURCE_DIR}/client" AND NOT ${DISABLE_CLIENT})

#ADD_SUBDIRECTORY(console)
ADD_SUBDIRECTORY(server)
ADD_SUBDIRECTORY(libtester)
#ADD_SUBDIRECTORY(updater)

# Add all the documentation targets to a single target "doc".
GET_PROPERTY(targets GLOBAL PROPERTY DOC_TARGETS)
ADD_CUSTOM_TARGET(doc DEPENDS ${targets})

#ADD_SUBDIRECTORY(docs)

IF(NOT BSD)
    #ADD_SUBDIRECTORY(features)
ENDIF(NOT BSD)

# On Linux system, install the icons where they can be found. These are used
# for the application menu launcher icons.
IF(NOT WIN32)
    INSTALL(FILES icons/16x16/comp_hack.png
        DESTINATION share/icons/hicolor/16x16/apps)
    INSTALL(FILES icons/22x22/comp_hack.png
        DESTINATION share/icons/hicolor/22x22/apps)
    INSTALL(FILES icons/24x24/comp_hack.png
        DESTINATION share/icons/hicolor/24x24/apps)
    INSTALL(FILES icons/32x32/comp_hack.png
        DESTINATION share/icons/hicolor/32x32/apps)
    INSTALL(FILES icons/48x48/comp_hack.png
        DESTINATION share/icons/hicolor/48x48/apps)
    INSTALL(FILES icons/64x64/comp_hack.png
        DESTINATION share/icons/hicolor/64x64/apps)
    INSTALL(FILES icons/64x64/comp_hack.png
        DESTINATION share/app-install/icons)
ENDIF(NOT WIN32)
