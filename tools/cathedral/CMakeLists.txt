# This file is part of COMP_hack.
#
# Copyright (C) 2010-2018 COMP_hack Team <compomega@tutanota.com>
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

PROJECT(comp_cathedral)

SET(CMAKE_AUTOMOC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

FIND_PACKAGE(Qt5Widgets REQUIRED)
FIND_PACKAGE(Qt5Xml REQUIRED)

MESSAGE("** Configuring ${PROJECT_NAME} **")

SET(${PROJECT_NAME}_SRCS
    src/main.cpp
    src/ActionUI.cpp
    src/ActionIDs.cpp
    src/ActionIDsItem.cpp
    src/ActionList.cpp
    src/ActionMap.cpp
    src/ActionMapItem.cpp
    src/ActionAddRemoveItemsUI.cpp
    src/ActionAddRemoveStatusUI.cpp
    src/ActionDisplayMessageUI.cpp
    src/ActionGrantSkillsUI.cpp
    src/ActionGrantXPUI.cpp
    src/ActionPlayBGMUI.cpp
    src/ActionPlaySoundEffectUI.cpp
    src/ActionSetHomepointUI.cpp
    src/ActionSetNPCStateUI.cpp
    src/ActionSpawnUI.cpp
    src/ActionSpecialDirectionUI.cpp
    src/ActionStageEffectUI.cpp
    src/ActionStartEventUI.cpp
    src/ActionUpdateCOMPUI.cpp
    src/ActionUpdateFlagUI.cpp
    src/ActionUpdateLNCUI.cpp
    src/ActionUpdatePointsUI.cpp
    src/ActionUpdateQuestUI.cpp
    src/ActionUpdateZoneFlagsUI.cpp
    src/ActionZoneChangeUI.cpp
    src/MainWindow.cpp
    src/NPCListWindow.cpp
    src/ObjectListModel.cpp
    src/ObjectListWindow.cpp
    src/SpotListWindow.cpp
)

SET(${PROJECT_NAME}_HDRS
    src/ActionUI.h
    src/ActionIDs.h
    src/ActionIDsItem.h
    src/ActionList.h
    src/ActionMap.h
    src/ActionMapItem.h
    src/ActionAddRemoveItemsUI.h
    src/ActionAddRemoveStatusUI.h
    src/ActionDisplayMessageUI.h
    src/ActionGrantSkillsUI.h
    src/ActionGrantXPUI.h
    src/ActionPlayBGMUI.h
    src/ActionPlaySoundEffectUI.h
    src/ActionSetHomepointUI.h
    src/ActionSetNPCStateUI.h
    src/ActionSpawnUI.h
    src/ActionSpecialDirectionUI.h
    src/ActionStageEffectUI.h
    src/ActionStartEventUI.h
    src/ActionUpdateCOMPUI.h
    src/ActionUpdateFlagUI.h
    src/ActionUpdateLNCUI.h
    src/ActionUpdatePointsUI.h
    src/ActionUpdateQuestUI.h
    src/ActionUpdateZoneFlagsUI.h
    src/ActionZoneChangeUI.h
    src/MainWindow.h
    src/NPCListWindow.h
    src/ObjectListModel.h
    src/ObjectListWindow.h
    src/SpotListWindow.h
)

SET(${PROJECT_NAME}_RCC
    res/cathedral.qrc
)
qt5_add_resources(${PROJECT_NAME}_RCC_SRCS ${${PROJECT_NAME}_RCC})

SET(${PROJECT_NAME}_UIS
    ui/Action.ui
    ui/ActionIDs.ui
    ui/ActionIDsItem.ui
    ui/ActionList.ui
    ui/ActionMap.ui
    ui/ActionMapItem.ui
    ui/ActionAddRemoveItems.ui
    ui/ActionAddRemoveStatus.ui
    ui/ActionDisplayMessage.ui
    ui/ActionGrantSkills.ui
    ui/ActionGrantXP.ui
    ui/ActionPlayBGM.ui
    ui/ActionPlaySoundEffect.ui
    ui/ActionSetHomepoint.ui
    ui/ActionSetNPCState.ui
    ui/ActionSpawn.ui
    ui/ActionSpecialDirection.ui
    ui/ActionStageEffect.ui
    ui/ActionStartEvent.ui
    ui/ActionUpdateCOMP.ui
    ui/ActionUpdateFlag.ui
    ui/ActionUpdateLNC.ui
    ui/ActionUpdatePoints.ui
    ui/ActionUpdateQuest.ui
    ui/ActionUpdateZoneFlags.ui
    ui/ActionZoneChange.ui
    ui/MainWindow.ui
    ui/ObjectListWindow.ui

    # Properties for each object.
    ui/NPCProperties.ui
    ui/SpotProperties.ui
)
qt5_wrap_ui(${PROJECT_NAME}_UI_SRCS ${${PROJECT_NAME}_UIS})

RES_WRAP(${PROJECT_NAME}_SRCS res/cathedral.rc)

ADD_EXECUTABLE(${PROJECT_NAME} WIN32 ${${PROJECT_NAME}_SRCS}
    ${${PROJECT_NAME}_HDRS} ${${PROJECT_NAME}_RCC_SRCS}
    ${${PROJECT_NAME}_UI_SRCS})

IF(MINGW)
    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS -mwindows)
ENDIF(MINGW)

UPX_WRAP(${PROJECT_NAME})

SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES FOLDER "Tools")

TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

TARGET_LINK_LIBRARIES(${PROJECT_NAME} comp Qt5::Widgets Qt5::Xml zlib)

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${COMP_INSTALL_DIR})

IF(NOT WIN32)
    INSTALL(FILES res/${PROJECT_NAME}.desktop DESTINATION share/applications)
ENDIF(NOT WIN32)
