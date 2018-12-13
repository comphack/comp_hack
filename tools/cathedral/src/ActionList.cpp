/**
 * @file tools/cathedral/src/ActionList.cpp
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Implementation for a list of actions.
 *
 * Copyright (C) 2012-2018 COMP_hack Team <compomega@tutanota.com>
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
#include "ActionUI.h"
#include "ActionAddRemoveItemsUI.h"
#include "ActionAddRemoveStatusUI.h"
#include "ActionDisplayMessageUI.h"
#include "ActionGrantSkillsUI.h"
#include "ActionGrantXPUI.h"
#include "ActionPlayBGMUI.h"
#include "ActionPlaySoundEffectUI.h"
#include "ActionSetHomepointUI.h"
#include "ActionSetNPCStateUI.h"
#include "ActionSpawnUI.h"
#include "ActionSpecialDirectionUI.h"
#include "ActionStageEffectUI.h"
#include "ActionStartEventUI.h"
#include "ActionUpdateCOMPUI.h"
#include "ActionUpdateFlagUI.h"
#include "ActionUpdateLNCUI.h"
#include "ActionUpdatePointsUI.h"
#include "ActionUpdateQuestUI.h"
#include "ActionUpdateZoneFlagsUI.h"
#include "ActionZoneChangeUI.h"
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

// Qt Includes
#include <PushIgnore.h>
#include <QAction>
#include <QMenu>

#include "ui_ActionList.h"
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

// Standard C++11 Includes
#include <algorithm>

ActionList::ActionList(QWidget *pParent) : QWidget(pParent)
{
    ui = new Ui::ActionList;
    ui->setupUi(this);

    QAction *pAction = nullptr;

    QMenu *pAddMenu = new QMenu(tr("Add"));

    pAction = pAddMenu->addAction("Add/Remove Items");
    pAction->setData(to_underlying(
        objects::Action::ActionType_t::ADD_REMOVE_ITEMS));
    connect(pAction, SIGNAL(triggered()), this, SLOT(AddNewAction()));

    pAction = pAddMenu->addAction("Add/Remove Status");
    pAction->setData(to_underlying(
        objects::Action::ActionType_t::ADD_REMOVE_STATUS));
    connect(pAction, SIGNAL(triggered()), this, SLOT(AddNewAction()));

    pAction = pAddMenu->addAction("Display Message");
    pAction->setData(to_underlying(
        objects::Action::ActionType_t::DISPLAY_MESSAGE));
    connect(pAction, SIGNAL(triggered()), this, SLOT(AddNewAction()));

    pAction = pAddMenu->addAction("Grant Skills");
    pAction->setData(to_underlying(
        objects::Action::ActionType_t::GRANT_SKILLS));
    connect(pAction, SIGNAL(triggered()), this, SLOT(AddNewAction()));

    pAction = pAddMenu->addAction("Grant XP");
    pAction->setData(to_underlying(
        objects::Action::ActionType_t::GRANT_XP));
    connect(pAction, SIGNAL(triggered()), this, SLOT(AddNewAction()));

    pAction = pAddMenu->addAction("Play BGM");
    pAction->setData(to_underlying(
        objects::Action::ActionType_t::PLAY_BGM));
    connect(pAction, SIGNAL(triggered()), this, SLOT(AddNewAction()));

    pAction = pAddMenu->addAction("Play Sound Effect");
    pAction->setData(to_underlying(
        objects::Action::ActionType_t::PLAY_SOUND_EFFECT));
    connect(pAction, SIGNAL(triggered()), this, SLOT(AddNewAction()));

    pAction = pAddMenu->addAction("Set Homepoint");
    pAction->setData(to_underlying(
        objects::Action::ActionType_t::SET_HOMEPOINT));
    connect(pAction, SIGNAL(triggered()), this, SLOT(AddNewAction()));

    pAction = pAddMenu->addAction("Set NPC State");
    pAction->setData(to_underlying(
        objects::Action::ActionType_t::SET_NPC_STATE));
    connect(pAction, SIGNAL(triggered()), this, SLOT(AddNewAction()));

    pAction = pAddMenu->addAction("Spawn");
    pAction->setData(to_underlying(
        objects::Action::ActionType_t::SPAWN));
    connect(pAction, SIGNAL(triggered()), this, SLOT(AddNewAction()));

    pAction = pAddMenu->addAction("Special Direction");
    pAction->setData(to_underlying(
        objects::Action::ActionType_t::SPECIAL_DIRECTION));
    connect(pAction, SIGNAL(triggered()), this, SLOT(AddNewAction()));

    pAction = pAddMenu->addAction("Stage Effect");
    pAction->setData(to_underlying(
        objects::Action::ActionType_t::STAGE_EFFECT));
    connect(pAction, SIGNAL(triggered()), this, SLOT(AddNewAction()));

    pAction = pAddMenu->addAction("Start Event");
    pAction->setData(to_underlying(
        objects::Action::ActionType_t::START_EVENT));
    connect(pAction, SIGNAL(triggered()), this, SLOT(AddNewAction()));

    pAction = pAddMenu->addAction("Update COMP");
    pAction->setData(to_underlying(
        objects::Action::ActionType_t::UPDATE_COMP));
    connect(pAction, SIGNAL(triggered()), this, SLOT(AddNewAction()));

    pAction = pAddMenu->addAction("Update Flag");
    pAction->setData(to_underlying(
        objects::Action::ActionType_t::UPDATE_FLAG));
    connect(pAction, SIGNAL(triggered()), this, SLOT(AddNewAction()));

    pAction = pAddMenu->addAction("Update LNC");
    pAction->setData(to_underlying(
        objects::Action::ActionType_t::UPDATE_LNC));
    connect(pAction, SIGNAL(triggered()), this, SLOT(AddNewAction()));

    pAction = pAddMenu->addAction("Update Points");
    pAction->setData(to_underlying(
        objects::Action::ActionType_t::UPDATE_POINTS));
    connect(pAction, SIGNAL(triggered()), this, SLOT(AddNewAction()));

    pAction = pAddMenu->addAction("Update Quest");
    pAction->setData(to_underlying(
        objects::Action::ActionType_t::UPDATE_QUEST));
    connect(pAction, SIGNAL(triggered()), this, SLOT(AddNewAction()));

    pAction = pAddMenu->addAction("Update Zone Flags");
    pAction->setData(to_underlying(
        objects::Action::ActionType_t::UPDATE_ZONE_FLAGS));
    connect(pAction, SIGNAL(triggered()), this, SLOT(AddNewAction()));

    pAction = pAddMenu->addAction("Zone Change");
    pAction->setData(to_underlying(
        objects::Action::ActionType_t::ZONE_CHANGE));
    connect(pAction, SIGNAL(triggered()), this, SLOT(AddNewAction()));

    ui->actionAdd->setMenu(pAddMenu);
}

ActionList::~ActionList()
{
    delete ui;
}

void ActionList::SetMainWindow(MainWindow *pMainWindow)
{
    mMainWindow = pMainWindow;
}

void ActionList::Load(const std::list<std::shared_ptr<
    objects::Action>>& actions)
{
    ClearActions();

    for(auto act : actions)
    {
        switch(act->GetActionType())
        {
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
            case objects::Action::ActionType_t::SPAWN:
                AddAction(act, new ActionSpawn(this, mMainWindow));
                break;
            default:
                /// @todo Error out here.
                break;
        }
    }

    RefreshPositions();
}

std::list<std::shared_ptr<objects::Action>> ActionList::Save() const
{
    std::list<std::shared_ptr<objects::Action>> actions;

    for(auto act : mActions)
    {
        actions.push_back(act->Save());
    }

    return actions;
}

void ActionList::AddAction(const std::shared_ptr<objects::Action>& act,
    Action *pAction)
{
    pAction->Load(act);

    mActions.push_back(pAction);

    ui->actionListLayout->insertWidget(ui->actionListLayout->count() - 1,
        pAction);
}

void ActionList::RemoveAction(Action *pAction)
{
    ui->actionListLayout->removeWidget(pAction);

    auto it = std::find(mActions.begin(), mActions.end(), pAction);

    if(mActions.end() != it)
    {
        mActions.erase(it);
    }

    pAction->deleteLater();
}

void ActionList::MoveUp(Action *pAction)
{
    (void)pAction;

    auto it = std::find(mActions.begin(), mActions.end(), pAction);

    if(mActions.end() == it)
    {
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
}

void ActionList::MoveDown(Action *pAction)
{
    (void)pAction;

    auto it = std::find(mActions.begin(), mActions.end(), pAction);

    if(mActions.end() == it)
    {
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
}

void ActionList::ClearActions()
{
    for(auto pAction : mActions)
    {
        ui->actionListLayout->removeWidget(pAction);

        delete pAction;
    }

    mActions.clear();
}

void ActionList::AddNewAction()
{
    QAction *pAction = qobject_cast<QAction*>(sender());

    if(!pAction)
    {
        return;
    }

    auto actionType = (objects::Action::ActionType_t)pAction->data().toUInt();

    switch(actionType)
    {
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
        case objects::Action::ActionType_t::SPAWN:
            AddAction(std::make_shared<objects::ActionSpawn>(),
                new ActionSpawn(this, mMainWindow));
            break;
        default:
            break;
    }

    RefreshPositions();
}

void ActionList::RefreshPositions()
{
    int i = 0;
    int last = (int)mActions.size() - 1;

    for(auto act : mActions)
    {
        act->UpdatePosition(0 == i, last == i);
        i++;
    }
}
