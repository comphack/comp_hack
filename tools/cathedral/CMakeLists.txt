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
    src/ActionList.cpp
    src/ActionMap.cpp
    src/ActionMapItem.cpp
    src/ActionAddRemoveItemsUI.cpp
    src/ActionAddRemoveStatusUI.cpp
    src/ActionCreateLootUI.cpp
    src/ActionDelayUI.cpp
    src/ActionDisplayMessageUI.cpp
    src/ActionGrantSkillsUI.cpp
    src/ActionGrantXPUI.cpp
    src/ActionPlayBGMUI.cpp
    src/ActionPlaySoundEffectUI.cpp
    src/ActionRunScriptUI.cpp
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
    src/ActionZoneInstanceUI.cpp
    src/BinaryDataNamedSet.cpp
    src/DynamicList.cpp
    src/DynamicListItem.cpp
    src/EventUI.cpp
    src/EventBaseUI.cpp
    src/EventChoiceUI.cpp
    src/EventConditionUI.cpp
    src/EventDirectionUI.cpp
    src/EventExNPCMessageUI.cpp
    src/EventITimeUI.cpp
    src/EventMessageRef.cpp
    src/EventMultitalkUI.cpp
    src/EventNPCMessageUI.cpp
    src/EventOpenMenuUI.cpp
    src/EventPerformActionsUI.cpp
    src/EventPlaySceneUI.cpp
    src/EventPromptUI.cpp
    src/EventRef.cpp
    src/EventWindow.cpp
    src/ItemDropUI.cpp
    src/MainWindow.cpp
    src/NPCList.cpp
    src/ObjectListModel.cpp
    src/ObjectList.cpp
    src/ObjectPositionUI.cpp
    src/ObjectSelector.cpp
    src/ObjectSelectorBase.cpp
    src/ObjectSelectorList.cpp
    src/ObjectSelectorWindow.cpp
    src/ServerScript.cpp
    src/SpotList.cpp
    src/ZoneWindow.cpp
)

SET(${PROJECT_NAME}_HDRS
    src/ActionUI.h
    src/ActionList.h
    src/ActionMap.h
    src/ActionMapItem.h
    src/ActionAddRemoveItemsUI.h
    src/ActionAddRemoveStatusUI.h
    src/ActionCreateLootUI.h
    src/ActionDelayUI.h
    src/ActionDisplayMessageUI.h
    src/ActionGrantSkillsUI.h
    src/ActionGrantXPUI.h
    src/ActionPlayBGMUI.h
    src/ActionPlaySoundEffectUI.h
    src/ActionRunScriptUI.h
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
    src/ActionZoneInstanceUI.h
    src/BinaryDataNamedSet.h
    src/DynamicList.h
    src/DynamicListItem.h
    src/EventUI.h
    src/EventBaseUI.h
    src/EventChoiceUI.h
    src/EventConditionUI.h
    src/EventDirectionUI.h
    src/EventExNPCMessageUI.h
    src/EventITimeUI.h
    src/EventMessageRef.h
    src/EventMultitalkUI.h
    src/EventNPCMessageUI.h
    src/EventOpenMenuUI.h
    src/EventPerformActionsUI.h
    src/EventPlaySceneUI.h
    src/EventPromptUI.h
    src/EventRef.h
    src/EventWindow.h
    src/ItemDropUI.h
    src/MainWindow.h
    src/NPCList.h
    src/ObjectListModel.h
    src/ObjectList.h
    src/ObjectPositionUI.h
    src/ObjectSelector.h
    src/ObjectSelectorBase.h
    src/ObjectSelectorList.h
    src/ObjectSelectorWindow.h
    src/ServerScript.h
    src/SpotList.h
    src/ZoneWindow.h
)

SET(${PROJECT_NAME}_RCC
    res/cathedral.qrc
)
qt5_add_resources(${PROJECT_NAME}_RCC_SRCS ${${PROJECT_NAME}_RCC})

SET(${PROJECT_NAME}_UIS
    ui/Action.ui
    ui/ActionList.ui
    ui/ActionMap.ui
    ui/ActionMapItem.ui
    ui/ActionAddRemoveItems.ui
    ui/ActionAddRemoveStatus.ui
    ui/ActionCreateLoot.ui
    ui/ActionDelay.ui
    ui/ActionDisplayMessage.ui
    ui/ActionGrantSkills.ui
    ui/ActionGrantXP.ui
    ui/ActionPlayBGM.ui
    ui/ActionPlaySoundEffect.ui
    ui/ActionRunScript.ui
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
    ui/ActionZoneInstance.ui
    ui/DynamicList.ui
    ui/DynamicListItem.ui
    ui/Event.ui
    ui/EventBase.ui
    ui/EventCondition.ui
    ui/EventDirection.ui
    ui/EventExNPCMessage.ui
    ui/EventITime.ui
    ui/EventMessageRef.ui
    ui/EventMultitalk.ui
    ui/EventNPCMessage.ui
    ui/EventOpenMenu.ui
    ui/EventPerformActions.ui
    ui/EventPlayScene.ui
    ui/EventPrompt.ui
    ui/EventRef.ui
    ui/EventWindow.ui
    ui/ItemDrop.ui
    ui/MainWindow.ui
    ui/ObjectList.ui
    ui/ObjectPosition.ui
    ui/ObjectSelector.ui
    ui/ObjectSelectorWindow.ui
    ui/ServerScript.ui
    ui/ZoneWindow.ui

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
