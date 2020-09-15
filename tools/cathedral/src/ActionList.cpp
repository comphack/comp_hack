/**
 * @file tools/cathedral/src/ActionList.cpp
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Implementation for a list of actions.
 *
 * Copyright (C) 2012-2020 COMP_hack Team <compomega@tutanota.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ActionList.h"

// Cathedral Includes
#include "ActionAddRemoveItemsUI.h"
#include "ActionAddRemoveStatusUI.h"
#include "ActionCreateLootUI.h"
#include "ActionDelayUI.h"
#include "ActionDisplayMessageUI.h"
#include "ActionGrantSkillsUI.h"
#include "ActionGrantXPUI.h"
#include "ActionPlayBGMUI.h"
#include "ActionPlaySoundEffectUI.h"
#include "ActionRunScriptUI.h"
#include "ActionSetHomepointUI.h"
#include "ActionSetNPCStateUI.h"
#include "ActionSpawnUI.h"
#include "ActionSpecialDirectionUI.h"
#include "ActionStageEffectUI.h"
#include "ActionStartEventUI.h"
#include "ActionUI.h"
#include "ActionUpdateCOMPUI.h"
#include "ActionUpdateFlagUI.h"
#include "ActionUpdateLNCUI.h"
#include "ActionUpdatePointsUI.h"
#include "ActionUpdateQuestUI.h"
#include "ActionUpdateZoneFlagsUI.h"
#include "ActionZoneChangeUI.h"
#include "ActionZoneInstanceUI.h"
#include "MainWindow.h"

// objects Includes
#include <ActionAddRemoveItems.h>
#include <ActionAddRemoveStatus.h>
#include <ActionDisplayMessage.h>
#include <ActionGrantSkills.h>
#include <ActionGrantXP.h>
#include <ActionPlayBGM.h>
#include <ActionPlaySoundEffect.h>
#include <ActionSetHomepoint.h>
#include <ActionSetNPCState.h>
#include <ActionSpawn.h>
#include <ActionSpecialDirection.h>
#include <ActionStageEffect.h>
#include <ActionStartEvent.h>
#include <ActionUpdateCOMP.h>
#include <ActionUpdateFlag.h>
#include <ActionUpdateLNC.h>
#include <ActionUpdatePoints.h>
#include <ActionUpdateQuest.h>
#include <ActionUpdateZoneFlags.h>
#include <ActionZoneChange.h>

// Ignore warnings
#include <PushIgnore.h>

// Qt Includes
#include <QAction>
#include <QMenu>

#include "ui_ActionList.h"

// Stop ignoring warnings
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

// Standard C++11 Includes
#include <algorithm>

ActionList::ActionList(QWidget *pParent) : QWidget(pParent) {
  ui = new Ui::ActionList;
  ui->setupUi(this);

  QAction *pAction = nullptr;

  QMenu *pAddMenu = new QMenu(tr("Add"));

  for (auto pair : GetActions()) {
    pAction = pAddMenu->addAction(qs(pair.first));
    pAction->setData(pair.second);
    connect(pAction, SIGNAL(triggered()), this, SLOT(AddNewAction()));
  }

  ui->actionAdd->setMenu(pAddMenu);
}

ActionList::~ActionList() { delete ui; }

void ActionList::SetMainWindow(MainWindow *pMainWindow) {
  mMainWindow = pMainWindow;
}

void ActionList::Load(
    const std::list<std::shared_ptr<objects::Action>> &actions) {
  ClearActions();

  for (auto act : actions) {
    switch (act->GetActionType()) {
      case objects::Action::ActionType_t::ZONE_CHANGE:
        AddAction(act, new ActionZoneChange(this, mMainWindow));
        break;
      case objects::Action::ActionType_t::START_EVENT:
        AddAction(act, new ActionStartEvent(this, mMainWindow));
        break;
      case objects::Action::ActionType_t::SET_HOMEPOINT:
        AddAction(act, new ActionSetHomepoint(this, mMainWindow));
        break;
      case objects::Action::ActionType_t::SET_NPC_STATE:
        AddAction(act, new ActionSetNPCState(this, mMainWindow));
        break;
      case objects::Action::ActionType_t::ADD_REMOVE_ITEMS:
        AddAction(act, new ActionAddRemoveItems(this, mMainWindow));
        break;
      case objects::Action::ActionType_t::ADD_REMOVE_STATUS:
        AddAction(act, new ActionAddRemoveStatus(this, mMainWindow));
        break;
      case objects::Action::ActionType_t::UPDATE_COMP:
        AddAction(act, new ActionUpdateCOMP(this, mMainWindow));
        break;
      case objects::Action::ActionType_t::GRANT_SKILLS:
        AddAction(act, new ActionGrantSkills(this, mMainWindow));
        break;
      case objects::Action::ActionType_t::GRANT_XP:
        AddAction(act, new ActionGrantXP(this, mMainWindow));
        break;
      case objects::Action::ActionType_t::DISPLAY_MESSAGE:
        AddAction(act, new ActionDisplayMessage(this, mMainWindow));
        break;
      case objects::Action::ActionType_t::STAGE_EFFECT:
        AddAction(act, new ActionStageEffect(this, mMainWindow));
        break;
      case objects::Action::ActionType_t::SPECIAL_DIRECTION:
        AddAction(act, new ActionSpecialDirection(this, mMainWindow));
        break;
      case objects::Action::ActionType_t::PLAY_BGM:
        AddAction(act, new ActionPlayBGM(this, mMainWindow));
        break;
      case objects::Action::ActionType_t::PLAY_SOUND_EFFECT:
        AddAction(act, new ActionPlaySoundEffect(this, mMainWindow));
        break;
      case objects::Action::ActionType_t::UPDATE_FLAG:
        AddAction(act, new ActionUpdateFlag(this, mMainWindow));
        break;
      case objects::Action::ActionType_t::UPDATE_LNC:
        AddAction(act, new ActionUpdateLNC(this, mMainWindow));
        break;
      case objects::Action::ActionType_t::UPDATE_POINTS:
        AddAction(act, new ActionUpdatePoints(this, mMainWindow));
        break;
      case objects::Action::ActionType_t::UPDATE_QUEST:
        AddAction(act, new ActionUpdateQuest(this, mMainWindow));
        break;
      case objects::Action::ActionType_t::UPDATE_ZONE_FLAGS:
        AddAction(act, new ActionUpdateZoneFlags(this, mMainWindow));
        break;
      case objects::Action::ActionType_t::ZONE_INSTANCE:
        AddAction(act, new ActionZoneInstance(this, mMainWindow));
        break;
      case objects::Action::ActionType_t::SPAWN:
        AddAction(act, new ActionSpawn(this, mMainWindow));
        break;
      case objects::Action::ActionType_t::CREATE_LOOT:
        AddAction(act, new ActionCreateLoot(this, mMainWindow));
        break;
      case objects::Action::ActionType_t::DELAY:
        AddAction(act, new ActionDelay(this, mMainWindow));
        break;
      case objects::Action::ActionType_t::RUN_SCRIPT:
        AddAction(act, new ActionRunScript(this, mMainWindow));
        break;
      default:
        // If we manage to get here, just don't add it
        break;
    }
  }

  RefreshPositions();
}

std::list<std::shared_ptr<objects::Action>> ActionList::Save() const {
  std::list<std::shared_ptr<objects::Action>> actions;

  for (auto act : mActions) {
    actions.push_back(act->Save());
  }

  return actions;
}

void ActionList::AddAction(const std::shared_ptr<objects::Action> &act,
                           Action *pAction) {
  pAction->Load(act);

  mActions.push_back(pAction);

  ui->actionListLayout->insertWidget(ui->actionListLayout->count() - 1,
                                     pAction);

  emit rowEdit();
}

void ActionList::RemoveAction(Action *pAction) {
  ui->actionListLayout->removeWidget(pAction);

  auto it = std::find(mActions.begin(), mActions.end(), pAction);

  if (mActions.end() != it) {
    mActions.erase(it);
  }

  pAction->deleteLater();

  RefreshPositions();

  emit rowEdit();
}

void ActionList::MoveUp(Action *pAction) {
  (void)pAction;

  auto it = std::find(mActions.begin(), mActions.end(), pAction);

  if (mActions.end() == it) {
    return;
  }

  auto before = it;
  before--;

  int idx = ui->actionListLayout->indexOf(*it);

  mActions.erase(it);
  mActions.insert(before, pAction);

  ui->actionListLayout->removeWidget(pAction);
  ui->actionListLayout->insertWidget(idx - 1, pAction);

  RefreshPositions();

  emit rowEdit();
}

void ActionList::MoveDown(Action *pAction) {
  (void)pAction;

  auto it = std::find(mActions.begin(), mActions.end(), pAction);

  if (mActions.end() == it) {
    return;
  }

  auto after = it;
  after++;
  after++;

  int idx = ui->actionListLayout->indexOf(*it);

  mActions.erase(it);
  mActions.insert(after, pAction);

  ui->actionListLayout->removeWidget(pAction);
  ui->actionListLayout->insertWidget(idx + 1, pAction);

  RefreshPositions();

  emit rowEdit();
}

std::list<std::pair<libcomp::String, int32_t>> ActionList::GetActions() {
  static std::list<std::pair<libcomp::String, int32_t>> actions = {
      {std::make_pair(libcomp::String("Add/Remove Items"),
                      (int32_t)objects::Action::ActionType_t::ADD_REMOVE_ITEMS),
       std::make_pair(
           libcomp::String("Add/Remove Status"),
           (int32_t)objects::Action::ActionType_t::ADD_REMOVE_STATUS),
       std::make_pair(libcomp::String("Create Loot"),
                      (int32_t)objects::Action::ActionType_t::CREATE_LOOT),
       std::make_pair(libcomp::String("Delay"),
                      (int32_t)objects::Action::ActionType_t::DELAY),
       std::make_pair(libcomp::String("Display Message"),
                      (int32_t)objects::Action::ActionType_t::DISPLAY_MESSAGE),
       std::make_pair(libcomp::String("Grant Skills"),
                      (int32_t)objects::Action::ActionType_t::GRANT_SKILLS),
       std::make_pair(libcomp::String("Grant XP"),
                      (int32_t)objects::Action::ActionType_t::GRANT_XP),
       std::make_pair(libcomp::String("Play BGM"),
                      (int32_t)objects::Action::ActionType_t::PLAY_BGM),
       std::make_pair(
           libcomp::String("Play Sound Effect"),
           (int32_t)objects::Action::ActionType_t::PLAY_SOUND_EFFECT),
       std::make_pair(libcomp::String("Run Script"),
                      (int32_t)objects::Action::ActionType_t::RUN_SCRIPT),
       std::make_pair(libcomp::String("Set Homepoint"),
                      (int32_t)objects::Action::ActionType_t::SET_HOMEPOINT),
       std::make_pair(libcomp::String("Set NPC State"),
                      (int32_t)objects::Action::ActionType_t::SET_NPC_STATE),
       std::make_pair(libcomp::String("Spawn"),
                      (int32_t)objects::Action::ActionType_t::SPAWN),
       std::make_pair(
           libcomp::String("Special Direction"),
           (int32_t)objects::Action::ActionType_t::SPECIAL_DIRECTION),
       std::make_pair(libcomp::String("Stage Effect"),
                      (int32_t)objects::Action::ActionType_t::STAGE_EFFECT),
       std::make_pair(libcomp::String("Start Event"),
                      (int32_t)objects::Action::ActionType_t::START_EVENT),
       std::make_pair(libcomp::String("Update COMP"),
                      (int32_t)objects::Action::ActionType_t::UPDATE_COMP),
       std::make_pair(libcomp::String("Update Flag"),
                      (int32_t)objects::Action::ActionType_t::UPDATE_FLAG),
       std::make_pair(libcomp::String("Update LNC"),
                      (int32_t)objects::Action::ActionType_t::UPDATE_LNC),
       std::make_pair(libcomp::String("Update Points"),
                      (int32_t)objects::Action::ActionType_t::UPDATE_POINTS),
       std::make_pair(libcomp::String("Update Quest"),
                      (int32_t)objects::Action::ActionType_t::UPDATE_QUEST),
       std::make_pair(
           libcomp::String("Update Zone Flags"),
           (int32_t)objects::Action::ActionType_t::UPDATE_ZONE_FLAGS),
       std::make_pair(libcomp::String("Zone Change"),
                      (int32_t)objects::Action::ActionType_t::ZONE_CHANGE),
       std::make_pair(libcomp::String("Zone Instance"),
                      (int32_t)objects::Action::ActionType_t::ZONE_INSTANCE)}};

  return actions;
}

void ActionList::ClearActions() {
  for (auto pAction : mActions) {
    ui->actionListLayout->removeWidget(pAction);

    delete pAction;
  }

  mActions.clear();

  emit rowEdit();
}

void ActionList::AddNewAction() {
  QAction *pAction = qobject_cast<QAction *>(sender());

  if (!pAction) {
    return;
  }

  auto actionType = (objects::Action::ActionType_t)pAction->data().toUInt();

  switch (actionType) {
    case objects::Action::ActionType_t::ZONE_CHANGE:
      AddAction(std::make_shared<objects::ActionZoneChange>(),
                new ActionZoneChange(this, mMainWindow));
      break;
    case objects::Action::ActionType_t::START_EVENT:
      AddAction(std::make_shared<objects::ActionStartEvent>(),
                new ActionStartEvent(this, mMainWindow));
      break;
    case objects::Action::ActionType_t::SET_HOMEPOINT:
      AddAction(std::make_shared<objects::ActionSetHomepoint>(),
                new ActionSetHomepoint(this, mMainWindow));
      break;
    case objects::Action::ActionType_t::SET_NPC_STATE:
      AddAction(std::make_shared<objects::ActionSetNPCState>(),
                new ActionSetNPCState(this, mMainWindow));
      break;
    case objects::Action::ActionType_t::ADD_REMOVE_ITEMS:
      AddAction(std::make_shared<objects::ActionAddRemoveItems>(),
                new ActionAddRemoveItems(this, mMainWindow));
      break;
    case objects::Action::ActionType_t::ADD_REMOVE_STATUS:
      AddAction(std::make_shared<objects::ActionAddRemoveStatus>(),
                new ActionAddRemoveStatus(this, mMainWindow));
      break;
    case objects::Action::ActionType_t::UPDATE_COMP:
      AddAction(std::make_shared<objects::ActionUpdateCOMP>(),
                new ActionUpdateCOMP(this, mMainWindow));
      break;
    case objects::Action::ActionType_t::GRANT_SKILLS:
      AddAction(std::make_shared<objects::ActionGrantSkills>(),
                new ActionGrantSkills(this, mMainWindow));
      break;
    case objects::Action::ActionType_t::GRANT_XP:
      AddAction(std::make_shared<objects::ActionGrantXP>(),
                new ActionGrantXP(this, mMainWindow));
      break;
    case objects::Action::ActionType_t::DISPLAY_MESSAGE:
      AddAction(std::make_shared<objects::ActionDisplayMessage>(),
                new ActionDisplayMessage(this, mMainWindow));
      break;
    case objects::Action::ActionType_t::STAGE_EFFECT:
      AddAction(std::make_shared<objects::ActionStageEffect>(),
                new ActionStageEffect(this, mMainWindow));
      break;
    case objects::Action::ActionType_t::SPECIAL_DIRECTION:
      AddAction(std::make_shared<objects::ActionSpecialDirection>(),
                new ActionSpecialDirection(this, mMainWindow));
      break;
    case objects::Action::ActionType_t::PLAY_BGM:
      AddAction(std::make_shared<objects::ActionPlayBGM>(),
                new ActionPlayBGM(this, mMainWindow));
      break;
    case objects::Action::ActionType_t::PLAY_SOUND_EFFECT:
      AddAction(std::make_shared<objects::ActionPlaySoundEffect>(),
                new ActionPlaySoundEffect(this, mMainWindow));
      break;
    case objects::Action::ActionType_t::UPDATE_FLAG:
      AddAction(std::make_shared<objects::ActionUpdateFlag>(),
                new ActionUpdateFlag(this, mMainWindow));
      break;
    case objects::Action::ActionType_t::UPDATE_LNC:
      AddAction(std::make_shared<objects::ActionUpdateLNC>(),
                new ActionUpdateLNC(this, mMainWindow));
      break;
    case objects::Action::ActionType_t::UPDATE_POINTS:
      AddAction(std::make_shared<objects::ActionUpdatePoints>(),
                new ActionUpdatePoints(this, mMainWindow));
      break;
    case objects::Action::ActionType_t::UPDATE_QUEST:
      AddAction(std::make_shared<objects::ActionUpdateQuest>(),
                new ActionUpdateQuest(this, mMainWindow));
      break;
    case objects::Action::ActionType_t::UPDATE_ZONE_FLAGS:
      AddAction(std::make_shared<objects::ActionUpdateZoneFlags>(),
                new ActionUpdateZoneFlags(this, mMainWindow));
      break;
    case objects::Action::ActionType_t::ZONE_INSTANCE:
      AddAction(std::make_shared<objects::ActionZoneInstance>(),
                new ActionZoneInstance(this, mMainWindow));
      break;
    case objects::Action::ActionType_t::SPAWN:
      AddAction(std::make_shared<objects::ActionSpawn>(),
                new ActionSpawn(this, mMainWindow));
      break;
    case objects::Action::ActionType_t::CREATE_LOOT:
      AddAction(std::make_shared<objects::ActionCreateLoot>(),
                new ActionCreateLoot(this, mMainWindow));
      break;
    case objects::Action::ActionType_t::DELAY:
      AddAction(std::make_shared<objects::ActionDelay>(),
                new ActionDelay(this, mMainWindow));
      break;
    case objects::Action::ActionType_t::RUN_SCRIPT:
      AddAction(std::make_shared<objects::ActionRunScript>(),
                new ActionRunScript(this, mMainWindow));
      break;
    default:
      break;
  }

  RefreshPositions();
}

void ActionList::RefreshPositions() {
  int i = 0;
  int last = (int)mActions.size() - 1;

  for (auto act : mActions) {
    act->UpdatePosition(0 == i, last == i);
    i++;
  }
}
