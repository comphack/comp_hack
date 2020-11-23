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

CMAKE_MINIMUM_REQUIRED(VERSION 3.5)

# Add the headless version as well.
ADD_SUBDIRECTORY(src)

IF(WIN32)

    PROJECT(comp_updater)

    SET(CMAKE_AUTOMOC ON)
    SET(CMAKE_INCLUDE_CURRENT_DIR ON)

    FIND_PACKAGE(Qt5Widgets REQUIRED)
    FIND_PACKAGE(Qt5Network REQUIRED)
    FIND_PACKAGE(Qt5WebEngine REQUIRED)
    FIND_PACKAGE(Qt5WebEngineWidgets REQUIRED)
    FIND_PACKAGE(Qt5Xml REQUIRED)
    FIND_PACKAGE(Qt5LinguistTools REQUIRED)

    MESSAGE("** Configuring ${PROJECT_NAME} **")

    CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/src/Version.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/Version.h @ONLY)

    SET(${PROJECT_NAME}_SRCS
        src/ClientPatches.cpp
        src/Downloader.cpp
        src/LanguageSelection.cpp
        src/Options.cpp
        src/Updater.cpp
        src/UpdaterWebEngineView.cpp
        src/main.cpp
    )

    SET(${PROJECT_NAME}_HDRS
        src/ClientPatches.h
        src/Downloader.h
        src/LanguageSelection.h
        src/Options.h
        src/Updater.h
        src/UpdaterWebEngineView.h
    )

    SET(${PROJECT_NAME}_UIS
        ui/LanguageSelection.ui
        ui/Options.ui
        ui/Updater.ui
    )
    qt5_wrap_ui(${PROJECT_NAME}_UI_SRCS ${${PROJECT_NAME}_UIS})

    qt5_create_translation(QM_FILES ${${PROJECT_NAME}_SRCS}
        ${${PROJECT_NAME}_HDRS} ${${PROJECT_NAME}_RCC_SRCS}
        ${${PROJECT_NAME}_UI_SRCS} trans/updater_en_US.ts
        trans/updater_ja_JP.ts trans/updater_ru_RU.ts)

    RES_WRAP(${PROJECT_NAME}_SRCS res/updater.rc)

    ADD_EXECUTABLE(${PROJECT_NAME} WIN32 ${${PROJECT_NAME}_SRCS}
        ${${PROJECT_NAME}_HDRS} ${${PROJECT_NAME}_RCC_SRCS}
        ${${PROJECT_NAME}_UI_SRCS} ${QM_FILES})

    ADD_QT_DEPS(${PROJECT_NAME})

    IF(MINGW)
        SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS -mwindows)
    ENDIF(MINGW)

    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER "Tools")

    TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PRIVATE
        ${LIBCOMP_INCLUDES}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_BINARY_DIR}
    )

    TARGET_LINK_LIBRARIES(${PROJECT_NAME} Qt5::Widgets Qt5::Network
        Qt5::WebEngineWidgets Qt5::Xml zlib)

    INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${COMP_INSTALL_DIR} COMPONENT tools)

    FOREACH(qm_file ${QM_FILES})
        INSTALL(FILES ${qm_file} DESTINATION translations COMPONENT tools)
    ENDFOREACH(qm_file)

ENDIF(WIN32)
