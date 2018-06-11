/**
 * @file server/channel/src/EventManager.cpp
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Manages the execution and processing of events as well
 *  as quest phase progression and condition evaluation.
 *
 * This file is part of the Channel Server (channel).
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

#include "EventManager.h"

// libcomp Includes
#include <DefinitionManager.h>
#include <Log.h>
#include <PacketCodes.h>
#include <Randomizer.h>
#include <ScriptEngine.h>
#include <ServerConstants.h>
#include <ServerDataManager.h>

// Standard C++11 Includes
#include <math.h>

// object Includes
#include <Account.h>
#include <ChannelConfig.h>
#include <CharacterProgress.h>
#include <DemonBox.h>
#include <DemonQuest.h>
#include <DemonQuestReward.h>
#include <DropSet.h>
#include <EventChoice.h>
#include <EventCondition.h>
#include <EventConditionData.h>
#include <EventDirection.h>
#include <EventExNPCMessage.h>
#include <EventFlagCondition.h>
#include <EventInstance.h>
#include <EventMultitalk.h>
#include <EventNPCMessage.h>
#include <EventOpenMenu.h>
#include <EventPerformActions.h>
#include <EventPlayScene.h>
#include <EventPrompt.h>
#include <EventScriptCondition.h>
#include <EventState.h>
#include <Expertise.h>
#include <Item.h>
#include <ItemBox.h>
#include <ItemDrop.h>
#include <MiDCategoryData.h>
#include <MiDevilBookData.h>
#include <MiDevilCrystalData.h>
#include <MiDevilData.h>
#include <MiEnchantData.h>
#include <MiExpertData.h>
#include <MiGrowthData.h>
#include <MiItemBasicData.h>
#include <MiItemData.h>
#include <MiQuestData.h>
#include <MiQuestPhaseData.h>
#include <MiQuestUpperCondition.h>
#include <MiSynthesisData.h>
#include <MiTriUnionSpecialData.h>
#include <MiUnionData.h>
#include <Party.h>
#include <Quest.h>
#include <QuestPhaseRequirement.h>
#include <ServerNPC.h>
#include <ServerObject.h>
#include <ServerZone.h>
#include <ServerZoneInstance.h>
#include <Spawn.h>
#include <TriFusionHostSession.h>
#include <WorldSharedConfig.h>

// channel Includes
#include "ActionManager.h"
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "CharacterState.h"
#include "FusionTables.h"
#include "ManagerConnection.h"
#include "TokuseiManager.h"
#include "ZoneInstance.h"
#include "ZoneManager.h"

using namespace channel;

const uint16_t EVENT_COMPARE_NUMERIC = (uint16_t)EventCompareMode::EQUAL |
    (uint16_t)EventCompareMode::LT | (uint16_t)EventCompareMode::GTE;

const uint16_t EVENT_COMPARE_NUMERIC2 = EVENT_COMPARE_NUMERIC |
    (uint16_t)EventCompareMode::BETWEEN;

struct channel::EventContext
{
    std::shared_ptr<ChannelClientConnection> Client;
    std::shared_ptr<Zone> CurrentZone;
    std::shared_ptr<objects::EventInstance> EventInstance;
};

EventManager::EventManager(const std::weak_ptr<ChannelServer>& server)
    : mServer(server)
{
}

EventManager::~EventManager()
{
}

bool EventManager::HandleEvent(
    const std::shared_ptr<ChannelClientConnection>& client,
    const libcomp::String& eventID, int32_t sourceEntityID,
    const std::shared_ptr<Zone>& zone, uint32_t actionGroupID)
{
    auto instance = PrepareEvent(eventID, sourceEntityID);
    if(instance)
    {
        instance->SetActionGroupID(actionGroupID);

        EventContext ctx;
        ctx.Client = client;
        ctx.EventInstance = instance;
        ctx.CurrentZone = client ? client->GetClientState()
            ->GetCharacterState()->GetZone() : zone;

        return HandleEvent(ctx);
    }

    return false;
}

std::shared_ptr<objects::EventInstance> EventManager::PrepareEvent(
    const libcomp::String& eventID, int32_t sourceEntityID)
{
    auto server = mServer.lock();
    auto serverDataManager = server->GetServerDataManager();

    auto event = serverDataManager->GetEventData(eventID);
    if(nullptr == event)
    {
        LOG_ERROR(libcomp::String("Invalid event ID encountered %1\n"
            ).Arg(eventID));
        return nullptr;
    }
    else
    {
        auto instance = std::shared_ptr<objects::EventInstance>(
            new objects::EventInstance);
        instance->SetEvent(event);
        instance->SetSourceEntityID(sourceEntityID);

        return instance;
    }
}

bool EventManager::HandleResponse(const std::shared_ptr<ChannelClientConnection>& client,
    int32_t responseID)
{
    auto state = client->GetClientState();
    auto eState = state->GetEventState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto current = eState->GetCurrent();

    if(nullptr == current)
    {
        LOG_ERROR(libcomp::String("Option selected for unknown event: %1\n")
            .Arg(state->GetAccountUID().ToString()));

        // End the event in case the client thinks something is actually happening
        EndEvent(client);
        return false;
    }

    auto event = current->GetEvent();
    auto eventType = event->GetEventType();
    switch(eventType)
    {
        case objects::Event::EventType_t::NPC_MESSAGE:
            if(responseID != 0)
            {
                LOG_ERROR("Non-zero response received for message response.\n");
            }
            else
            {
                auto e = std::dynamic_pointer_cast<objects::EventNPCMessage>(event);

                // If there are still more messages, increment and continue the same event
                if(current->GetIndex() < (e->MessageIDsCount() - 1))
                {
                    current->SetIndex((uint8_t)(current->GetIndex() + 1));
                    HandleEvent(client, current);
                    return true;
                }

                /// @todo: check infinite loops
            }
            break;
        case objects::Event::EventType_t::PROMPT:
            {
                auto e = std::dynamic_pointer_cast<objects::EventPrompt>(event);

                int32_t adjustedResponseID = responseID;
                for(size_t i = 0; i < e->ChoicesCount() && i <= (size_t)adjustedResponseID; i++)
                {
                    if(current->DisabledChoicesContains((uint8_t)i))
                    {
                        adjustedResponseID++;
                    }
                }

                auto choice = e->GetChoices((size_t)adjustedResponseID);
                if(choice == nullptr)
                {
                    LOG_ERROR(libcomp::String("Invalid choice %1 selected for event %2\n"
                        ).Arg(responseID).Arg(e->GetID()));
                }
                else
                {
                    current->SetState(choice);
                }
            }
            break;
        case objects::Event::EventType_t::OPEN_MENU:
        case objects::Event::EventType_t::PLAY_SCENE:
        case objects::Event::EventType_t::DIRECTION:
        case objects::Event::EventType_t::EX_NPC_MESSAGE:
        case objects::Event::EventType_t::MULTITALK:
            {
                if(responseID != 0)
                {
                    LOG_ERROR(libcomp::String("Non-zero response %1 received for event %2\n"
                        ).Arg(responseID).Arg(event->GetID()));
                }
            }
            break;
        default:
            LOG_ERROR(libcomp::String("Response received for invalid event of type %1\n"
                ).Arg(to_underlying(eventType)));
            break;
    }

    EventContext ctx;
    ctx.Client = client;
    ctx.EventInstance = current;
    ctx.CurrentZone = cState->GetZone();
    
    HandleNext(ctx);

    return true;
}

bool EventManager::UpdateQuest(const std::shared_ptr<ChannelClientConnection>& client,
    int16_t questID, int8_t phase, bool forceUpdate, const std::unordered_map<
    int32_t, int32_t>& updateFlags)
{
    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto questData = definitionManager->GetQuestData((uint32_t)questID);

    if(!questData)
    {
        LOG_ERROR(libcomp::String("Invalid quest ID supplied for UpdateQuest: %1\n")
            .Arg(questID));
        return false;
    }
    else if((phase < -1 && ! forceUpdate) || phase < -2 ||
        phase > (int8_t)questData->GetPhaseCount())
    {
        LOG_ERROR(libcomp::String("Invalid phase '%1' supplied for quest: %2\n")
            .Arg(phase).Arg(questID));
        return false;
    }

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto progress = character->GetProgress().Get();

    size_t index;
    uint8_t shiftVal;
    CharacterManager::ConvertIDToMaskValues((uint16_t)questID, index, shiftVal);

    uint8_t indexVal = progress->GetCompletedQuests(index);
    bool completed = (shiftVal & indexVal) != 0;

    auto dbChanges = libcomp::DatabaseChangeSet::Create(state->GetAccountUID());
    auto quest = character->GetQuests(questID).Get();
    bool sendUpdate = phase != -2;
    bool recalcCharacter = false;
    if(phase == -1)
    {
        // Completing a quest
        if(!quest && completed && !forceUpdate)
        {
            LOG_ERROR(libcomp::String("Quest '%1' has already been completed\n")
                .Arg(questID));
            return false;
        }

        recalcCharacter = cState->UpdateQuestState(definitionManager,
            (uint32_t)questID);

        dbChanges->Update(progress);

        if(quest)
        {
            character->RemoveQuests(questID);
            dbChanges->Update(character);
            dbChanges->Delete(quest);
        }
    }
    else if(phase == -2)
    {
        // Removing a quest
        progress->SetCompletedQuests(index, (uint8_t)(~shiftVal & indexVal));
        dbChanges->Update(progress);

        if(quest)
        {
            character->RemoveQuests(questID);
            dbChanges->Update(character);
            dbChanges->Delete(quest);

            SendActiveQuestList(client);
        }

        SendCompletedQuestList(client);

        recalcCharacter = cState->UpdateQuestState(definitionManager);
    }
    else if(!quest)
    {
        // Starting a quest
        if(!forceUpdate && completed && questData->GetType() != 1)
        {
            LOG_ERROR(libcomp::String("Already completed non-repeatable quest '%1'"
                " cannot be started again\n").Arg(questID));
            return false;
        }

        quest = libcomp::PersistentObject::New<objects::Quest>(true);
        quest->SetQuestID(questID);
        quest->SetCharacter(character->GetUUID());
        quest->SetPhase(phase);
        quest->SetFlagStates(updateFlags);

        character->SetQuests(questID, quest);
        dbChanges->Insert(quest);
        dbChanges->Update(character);
    }
    else if(phase == 0)
    {
        // If the quest already existed and we're not setting the phase,
        // check if we're setting the flags instead
        if(updateFlags.size() > 0)
        {
            sendUpdate = false;

            for(auto pair : updateFlags)
            {
                quest->SetFlagStates(pair.first, pair.second);
            }

            dbChanges->Update(quest);
        }
        else
        {
            return true;
        }
    }
    else
    {
        // Updating a quest phase
        if(!forceUpdate && quest->GetPhase() >= phase)
        {
            // Nothing to do but not an error
            return true;
        }

        quest->SetPhase(phase);
        
        // Keep the last phase's flags but set any that are new
        for(auto pair : updateFlags)
        {
            quest->SetFlagStates(pair.first, pair.second);
        }

        // Reset all the custom data
        for(size_t i = 0; i < quest->CustomDataCount(); i++)
        {
            quest->SetCustomData(i, 0);
        }

        dbChanges->Update(quest);
    }

    server->GetWorldDatabase()->QueueChangeSet(dbChanges);

    if(sendUpdate)
    {
        UpdateQuestTargetEnemies(client);

        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_QUEST_PHASE_UPDATE);
        p.WriteS16Little(questID);
        p.WriteS8(phase);

        client->SendPacket(p);
    }

    if(recalcCharacter)
    {
        server->GetTokuseiManager()->Recalculate(cState, true,
            std::set<int32_t>{ cState->GetEntityID() });

        // Always recalculate stats
        server->GetCharacterManager()->RecalculateStats(cState, client);
    }

    return true;
}

void EventManager::UpdateQuestKillCount(const std::shared_ptr<ChannelClientConnection>& client,
    const std::unordered_map<uint32_t, int32_t>& kills)
{
    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    std::set<int16_t> countUpdates;
    for(auto qPair : character->GetQuests())
    {
        auto quest = qPair.second.Get();
        auto questData = definitionManager->GetQuestData((uint32_t)qPair.first);
        int8_t currentPhase = quest ? quest->GetPhase() : -1;
        if(currentPhase < 0 || (int8_t)questData->GetPhaseCount() < currentPhase)
        {
            continue;
        }

        auto phaseData = questData->GetPhases((size_t)currentPhase);
        for(uint32_t i = 0; i < phaseData->GetRequirementCount(); i++)
        {
            auto req = phaseData->GetRequirements((size_t)i);

            auto it = kills.find(req->GetObjectID());
            if((req->GetType() == objects::QuestPhaseRequirement::Type_t::KILL ||
                req->GetType() == objects::QuestPhaseRequirement::Type_t::KILL_HIDDEN) &&
                it != kills.end())
            {
                int32_t customData = quest->GetCustomData((size_t)i);
                if(customData < (int32_t)req->GetObjectCount())
                {
                    customData = (int32_t)(customData + it->second);
                    if(customData > (int32_t)req->GetObjectCount())
                    {
                        customData = (int32_t)req->GetObjectCount();
                    }

                    countUpdates.insert(qPair.first);
                    quest->SetCustomData((size_t)i, customData);
                }
            }
        }

        if(countUpdates.size() > 0)
        {
            server->GetWorldDatabase()->QueueUpdate(quest, state->GetAccountUID());
        }
    }

    if(countUpdates.size() > 0)
    {
        for(int16_t questID : countUpdates)
        {
            auto quest = character->GetQuests(questID).Get();
            auto customData = quest->GetCustomData();

            libcomp::Packet p;
            p.WritePacketCode(
                ChannelToClientPacketCode_t::PACKET_QUEST_KILL_COUNT_UPDATE);
            p.WriteS16Little(questID);
            p.WriteArray(&customData, (uint32_t)customData.size() * sizeof(int32_t));

            client->QueuePacket(p);
        }

        client->FlushOutgoing();
    }

    // Update demon kill quest
    auto dQuest = character->GetDemonQuest().Get();
    if(dQuest)
    {
        for(auto& targetPair : dQuest->GetTargets())
        {
            auto it = kills.find(targetPair.first);
            if(it != kills.end() &&
                dQuest->GetType() == objects::DemonQuest::Type_t::KILL)
            {
                UpdateDemonQuestCount(client, dQuest->GetType(),
                    it->first, it->second);
            }
        }
    }
}

bool EventManager::EvaluateQuestConditions(EventContext& ctx, int16_t questID)
{
    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto questData = definitionManager->GetQuestData((uint32_t)questID);

    if(!questData)
    {
        LOG_ERROR(libcomp::String("Invalid quest ID supplied for"
            " EvaluateQuestConditions: %1\n").Arg(questID));
        return false;
    }
    else if(!questData->GetConditionsExist())
    {
        return true;
    }

    // Condition sets are handled as "or" checks so if any set passes,
    // the condition evaluates to true
    for(auto conditionSet : questData->GetConditions())
    {
        uint32_t clauseCount = conditionSet->GetClauseCount();
        bool passed = clauseCount > 0;
        for(uint32_t i = 0; i < clauseCount; i++)
        {
            if(!EvaluateCondition(ctx, conditionSet->GetClauses((size_t)i)))
            {
                passed = false;
                break;
            }
        }

        if(passed)
        {
            return true;
        }
    }

    return false;
}

bool EventManager::EvaluateEventCondition(EventContext& ctx, const std::shared_ptr<
    objects::EventCondition>& condition)
{
    auto client = ctx.Client;
    bool negate = condition->GetNegate();
    switch(condition->GetType())
    {
    case objects::EventCondition::Type_t::SCRIPT:
        {
            auto scriptCondition = std::dynamic_pointer_cast<
                objects::EventScriptCondition>(condition);
            if(!scriptCondition)
            {
                LOG_ERROR("Invalid event condition of type 'SCRIPT' encountered\n");
                return false;
            }

            auto serverDataManager = mServer.lock()->GetServerDataManager();
            auto script = serverDataManager->GetScript(scriptCondition->GetScriptID());
            if(script && script->Type.ToLower() == "eventcondition")
            {
                auto engine = std::make_shared<libcomp::ScriptEngine>();
                engine->Using<CharacterState>();
                engine->Using<DemonState>();
                engine->Using<Zone>();
                engine->Using<libcomp::Randomizer>();

                if(engine->Eval(script->Source))
                {
                    Sqrat::Function f(Sqrat::RootTable(engine->GetVM()), "check");

                    Sqrat::Array sqParams(engine->GetVM());
                    for(libcomp::String p : scriptCondition->GetParams())
                    {
                        sqParams.Append(p);
                    }

                    auto state = client ? client->GetClientState() : nullptr;
                    auto scriptResult = !f.IsNull()
                        ? f.Evaluate<int32_t>(
                            state ? state->GetCharacterState() : nullptr,
                            state ? state->GetDemonState() : nullptr,
                            ctx.CurrentZone,
                            scriptCondition->GetValue1(),
                            scriptCondition->GetValue2(),
                            sqParams) : 0;
                    if(scriptResult)
                    {
                        return negate != (*scriptResult == 0);
                    }
                }
            }
            else
            {
                LOG_ERROR(libcomp::String("Invalid event condition script ID: %1\n")
                    .Arg(scriptCondition->GetScriptID()));
            }
        }
        break;
    case objects::EventCondition::Type_t::ZONE_FLAGS:
    case objects::EventCondition::Type_t::ZONE_CHARACTER_FLAGS:
    case objects::EventCondition::Type_t::ZONE_INSTANCE_FLAGS:
    case objects::EventCondition::Type_t::ZONE_INSTANCE_CHARACTER_FLAGS:
        {
            int32_t worldCID = 0;
            bool instanceCheck = false;
            switch(condition->GetType())
            {
            case objects::EventCondition::Type_t::ZONE_FLAGS:
                break;
            case objects::EventCondition::Type_t::ZONE_CHARACTER_FLAGS:
                if(client)
                {
                    worldCID = client->GetClientState()->GetWorldCID();
                }
                else
                {
                    LOG_ERROR("Attempted to set zone character"
                        " flags with no associated client: %1\n");
                    return false;
                }
                break;
            case objects::EventCondition::Type_t::ZONE_INSTANCE_FLAGS:
                instanceCheck = true;
                break;
            case objects::EventCondition::Type_t::ZONE_INSTANCE_CHARACTER_FLAGS:
                if(client)
                {
                    instanceCheck = true;
                    worldCID = client->GetClientState()->GetWorldCID();
                }
                else
                {
                    LOG_ERROR("Attempted to set zone instance character"
                        " flags with no associated client: %1\n");
                    return false;
                }
                break;
            default:
                break;
            }

            auto zone = ctx.CurrentZone;
            auto flagCon = std::dynamic_pointer_cast<
                objects::EventFlagCondition>(condition);
            if(zone && flagCon)
            {
                std::unordered_map<int32_t, int32_t> flagStates;
                if(instanceCheck)
                {
                    auto inst = zone->GetInstance();
                    if(inst)
                    {
                        for(auto pair : flagCon->GetFlagStates())
                        {
                            int32_t val;
                            if(inst->GetFlagState(pair.first, val, worldCID))
                            {
                                flagStates[pair.first] = val;
                            }
                        }
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                {
                    for(auto pair : flagCon->GetFlagStates())
                    {
                        int32_t val;
                        if(zone->GetFlagState(pair.first, val, worldCID))
                        {
                            flagStates[pair.first] = val;
                        }
                    }
                }

                return negate != EvaluateFlagStates(flagStates, flagCon);
            }
        }
        break;
    case objects::EventCondition::Type_t::PARTNER_ALIVE:
    case objects::EventCondition::Type_t::PARTNER_FAMILIARITY:
    case objects::EventCondition::Type_t::PARTNER_LEVEL:
    case objects::EventCondition::Type_t::PARTNER_LOCKED:
    case objects::EventCondition::Type_t::PARTNER_SKILL_LEARNED:
    case objects::EventCondition::Type_t::PARTNER_STAT_VALUE:
    case objects::EventCondition::Type_t::SOUL_POINTS:
        return negate != (client && EvaluatePartnerCondition(client, condition));
    case objects::EventCondition::Type_t::QUEST_AVAILABLE:
    case objects::EventCondition::Type_t::QUEST_PHASE:
    case objects::EventCondition::Type_t::QUEST_PHASE_REQUIREMENTS:
    case objects::EventCondition::Type_t::QUEST_FLAGS:
        return negate != (client && EvaluateQuestCondition(ctx, condition));
    default:
        return negate != EvaluateCondition(ctx, condition,
            condition->GetCompareMode());
    }

    // Always return false when invalid
    return false;
}

bool EventManager::EvaluatePartnerCondition(const std::shared_ptr<
    ChannelClientConnection>& client, const std::shared_ptr<
    objects::EventCondition>& condition)
{
    auto state = client->GetClientState();
    auto dState = state->GetDemonState();
    auto demon = dState->GetEntity();

    if(!demon)
    {
        return false;
    }
    
    auto compareMode = condition->GetCompareMode();
    switch(condition->GetType())
    {
    case objects::EventCondition::Type_t::PARTNER_ALIVE:
        {
            // Partner is alive
            return (compareMode == EventCompareMode::EQUAL ||
                compareMode == EventCompareMode::DEFAULT_COMPARE) && dState->IsAlive();
        }
    case objects::EventCondition::Type_t::PARTNER_FAMILIARITY:
        {
            // Partner familiarity compares to [value 1] (and [value 2])
            return Compare((int32_t)demon->GetFamiliarity(), condition->GetValue1(),
                condition->GetValue2(), compareMode, EventCompareMode::GTE,
                EVENT_COMPARE_NUMERIC2);
        }
    case objects::EventCondition::Type_t::PARTNER_LEVEL:
        {
            // Partner level compares to [value 1] (and [value 2])
            auto stats = demon->GetCoreStats();

            return Compare(stats->GetLevel(), condition->GetValue1(),
                condition->GetValue2(), compareMode, EventCompareMode::GTE,
                EVENT_COMPARE_NUMERIC2);
        }
    case objects::EventCondition::Type_t::PARTNER_LOCKED:
        {
            // Partner is locked
            return (compareMode == EventCompareMode::EQUAL ||
                compareMode == EventCompareMode::DEFAULT_COMPARE) && demon->GetLocked();
        }
    case objects::EventCondition::Type_t::PARTNER_SKILL_LEARNED:
        {
            // Partner currently knows skill with ID [value 1]
            return (compareMode == EventCompareMode::EQUAL ||
                compareMode == EventCompareMode::DEFAULT_COMPARE) &&
                dState->CurrentSkillsContains((uint32_t)condition->GetValue1());
        }
    case objects::EventCondition::Type_t::PARTNER_STAT_VALUE:
        {
            // Partner stat at correct index [value 1] compares to [value 2]
            return Compare((int32_t)dState->GetCorrectValue(
                (CorrectTbl)condition->GetValue1()), condition->GetValue2(), 0,
                compareMode, EventCompareMode::GTE, EVENT_COMPARE_NUMERIC);
        }
    case objects::EventCondition::Type_t::SOUL_POINTS:
        {
            // Partner soul point amount compares to [value 1] (and [value 2])
            return Compare(demon->GetSoulPoints(), condition->GetValue1(),
                condition->GetValue2(), compareMode, EventCompareMode::GTE,
                EVENT_COMPARE_NUMERIC2);
        }
        break;
    default:
        break;
    }

    return false;
}

bool EventManager::EvaluateQuestCondition(EventContext& ctx,
    const std::shared_ptr<objects::EventCondition>& condition)
{
    if(!ctx.Client)
    {
        return false;
    }

    auto state = ctx.Client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    int16_t questID = (int16_t)condition->GetValue1();
    auto quest = character->GetQuests(questID).Get();

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto questData = definitionManager->GetQuestData((uint32_t)questID);

    auto compareMode = condition->GetCompareMode();
    switch(condition->GetType())
    {
    case objects::EventCondition::Type_t::QUEST_AVAILABLE:
        {
            // If the quest is active or completed and not-repeatable, it is not available
            // If neither of those are true, eveluate its starting conditions
            auto progress = character->GetProgress();

            size_t index;
            uint8_t shiftVal;
            CharacterManager::ConvertIDToMaskValues((uint16_t)questID,
                index, shiftVal);

            uint8_t indexVal = progress->GetCompletedQuests(index);
            bool completed = (shiftVal & indexVal) != 0;

            return quest == nullptr && (!completed || questData->GetType() == 1)
                && EvaluateQuestConditions(ctx, questID);
        }
    case objects::EventCondition::Type_t::QUEST_PHASE:
        if(quest)
        {
            return Compare((int32_t)quest->GetPhase(), condition->GetValue2(), 0,
                compareMode, EventCompareMode::EQUAL, EVENT_COMPARE_NUMERIC);
        }
        else if(compareMode == EventCompareMode::GTE)
        {
            // Count complete as true
            size_t index;
            uint8_t shiftVal;
            CharacterManager::ConvertIDToMaskValues((uint16_t)questID,
                index, shiftVal);

            uint8_t indexVal = character->GetProgress()->GetCompletedQuests(index);

            return (indexVal & shiftVal) != 0;
        }
        else
        {
            return compareMode == EventCompareMode::LT ||
                compareMode == EventCompareMode::LT_OR_NAN;
        }
    case objects::EventCondition::Type_t::QUEST_PHASE_REQUIREMENTS:
        return quest && EvaluateQuestPhaseRequirements(ctx.Client, questID,
            (int8_t)condition->GetValue2());
    case objects::EventCondition::Type_t::QUEST_FLAGS:
        if(!quest || ((int8_t)condition->GetValue2() > -1 &&
            quest->GetPhase() != (int8_t)condition->GetValue2()))
        {
            return false;
        }
        else
        {
            auto flagStates = quest->GetFlagStates();
            auto flagCon = std::dynamic_pointer_cast<objects::EventFlagCondition>(condition);

            return EvaluateFlagStates(flagStates, flagCon);
        }
        break;
    default:
        break;
    }

    return false;
}

bool EventManager::EvaluateFlagStates(const std::unordered_map<int32_t,
    int32_t>& flagStates, const std::shared_ptr<
    objects::EventFlagCondition>& condition)
{
    if(!condition)
    {
        LOG_ERROR("Invalid event flag condition encountered\n");
        return false;
    }

    bool result = true;
    switch(condition->GetCompareMode())
    {
    case EventCompareMode::EXISTS:
        for(auto pair : condition->GetFlagStates())
        {
            if(flagStates.find(pair.first) == flagStates.end())
            {
                result = false;
                break;
            }
        }
        break;
    case EventCompareMode::LT_OR_NAN:
        // Flag specific less than or not a number (does not exist)
        for(auto pair : condition->GetFlagStates())
        {
            auto it = flagStates.find(pair.first);
            if(it != flagStates.end() && it->second >= pair.second)
            {
                result = false;
                break;
            }
        }
        break;
    case EventCompareMode::LT:
        for(auto pair : condition->GetFlagStates())
        {
            auto it = flagStates.find(pair.first);
            if(it == flagStates.end() || it->second >= pair.second)
            {
                result = false;
                break;
            }
        }
        break;
    case EventCompareMode::GTE:
        for(auto pair : condition->GetFlagStates())
        {
            auto it = flagStates.find(pair.first);
            if(it == flagStates.end() || it->second < pair.second)
            {
                result = false;
                break;
            }
        }
        break;
    case EventCompareMode::DEFAULT_COMPARE:
    case EventCompareMode::EQUAL:
    default:
        for(auto pair : condition->GetFlagStates())
        {
            auto it = flagStates.find(pair.first);
            if(it == flagStates.end() || it->second != pair.second)
            {
                result = false;
                break;
            }
        }
        break;
    }

    return result;
}

bool EventManager::Compare(int32_t value1, int32_t value2, int32_t value3,
    EventCompareMode compareMode, EventCompareMode defaultCompare,
    uint16_t validCompareSetting)
{
    if(compareMode == EventCompareMode::DEFAULT_COMPARE)
    {
        if(defaultCompare == EventCompareMode::DEFAULT_COMPARE)
        {
            LOG_ERROR("Default comparison specified for non-defaulted"
                " comparison\n");
            return false;
        }

        compareMode = defaultCompare;
    }

    if(compareMode == EventCompareMode::EXISTS)
    {
        LOG_ERROR("EXISTS mode is not valid for generic comparison\n");
        return false;
    }

    if((validCompareSetting & (uint16_t)compareMode) == 0)
    {
        LOG_ERROR(libcomp::String("Invalid comparison mode attempted: %1\n")
            .Arg((int32_t)compareMode));
        return false;
    }

    switch(compareMode)
    {
    case EventCompareMode::EQUAL:
        return value1 == value2;
    case EventCompareMode::LT_OR_NAN:
        LOG_WARNING("LT_OR_NAN mode used generic comparison\n");
        return value1 < value2;
    case EventCompareMode::LT:
        return value1 < value2;
    case EventCompareMode::GTE:
        return value1 >= value2;
    case EventCompareMode::BETWEEN:
        return value1 >= value2 && value1 <= value3;
    default:
        return false;
    }
}

bool EventManager::EvaluateEventConditions(EventContext& ctx,
    const std::list<std::shared_ptr<objects::EventCondition>>& conditions)
{
    for(auto condition : conditions)
    {
        if(!EvaluateEventCondition(ctx, condition))
        {
            return false;
        }
    }

    return true;
}

bool EventManager::EvaluateCondition(EventContext& ctx,
    const std::shared_ptr<objects::EventConditionData>& condition,
    EventCompareMode compareMode)
{
    auto client = ctx.Client;

    switch(condition->GetType())
    {
    case objects::EventConditionData::Type_t::LEVEL:
        if(!client)
        {
            return false;
        }
        else
        {
            // Character level compares to [value 1] (and [value 2])
            auto character = client->GetClientState()->GetCharacterState()->GetEntity();
            auto stats = character->GetCoreStats();

            return Compare(stats->GetLevel(), condition->GetValue1(),
                condition->GetValue2(), compareMode, EventCompareMode::GTE,
                EVENT_COMPARE_NUMERIC2);
        }
    case objects::EventConditionData::Type_t::LNC_TYPE:
        if(!client || (compareMode != EventCompareMode::EQUAL &&
            compareMode != EventCompareMode::DEFAULT_COMPARE))
        {
            return false;
        }
        else
        {
            // Character LNC type matches [value 1]
            return client->GetClientState()->GetCharacterState()
                ->IsLNCType((uint8_t)condition->GetValue1(), false);
        }
    case objects::EventConditionData::Type_t::ITEM:
        if(!client)
        {
            return false;
        }
        else
        {
            // Item of type = [value 1] quantity compares to
            // [value 2] in the character's inventory
            auto character = client->GetClientState()->GetCharacterState()->GetEntity();
            uint32_t count = mServer.lock()->GetCharacterManager()
                ->GetExistingItemCount(character, (uint32_t)condition->GetValue1());

            return Compare((int32_t)count, condition->GetValue2(), 0,
                compareMode, EventCompareMode::GTE, EVENT_COMPARE_NUMERIC);
        }
    case objects::EventConditionData::Type_t::VALUABLE:
        if(!client || (compareMode != EventCompareMode::EQUAL &&
            compareMode != EventCompareMode::DEFAULT_COMPARE))
        {
            return false;
        }
        else
        {
            // Valuable flag [value 1] = [value 2]
            auto character = client->GetClientState()->GetCharacterState()->GetEntity();

            uint16_t valuableID = (uint16_t)condition->GetValue1();

            return CharacterManager::HasValuable(character, valuableID) !=
                (condition->GetValue2() == 0);
        }
    case objects::EventConditionData::Type_t::QUEST_COMPLETE:
        if(!client || (compareMode != EventCompareMode::EQUAL &&
            compareMode != EventCompareMode::DEFAULT_COMPARE))
        {
            return false;
        }
        else
        {
            // Complete quest flag [value 1] = [value 2]
            auto character = client->GetClientState()->GetCharacterState()->GetEntity();
            auto progress = character->GetProgress().Get();

            uint16_t questID = (uint16_t)condition->GetValue1();

            size_t index;
            uint8_t shiftVal;
            CharacterManager::ConvertIDToMaskValues(questID, index, shiftVal);

            uint8_t indexVal = progress->GetCompletedQuests(index);

            return ((indexVal & shiftVal) == 0) == (condition->GetValue2() == 0);
        }
    case objects::EventConditionData::Type_t::TIMESPAN:
        if(compareMode != EventCompareMode::BETWEEN && compareMode != EventCompareMode::DEFAULT_COMPARE)
        {
            return false;
        }
        else
        {
            // Server time between [value 1] and [value 2] (format: HHmm)
            auto clock = mServer.lock()->GetWorldClockTime();

            int8_t minHours = (int8_t)floorl((float)condition->GetValue1() * 0.01);
            int8_t minMinutes = (int8_t)(condition->GetValue1() - (minHours * 100));

            int8_t maxHours = (int8_t)floorl((float)condition->GetValue2() * 0.01);
            int8_t maxMinutes = (int8_t)(condition->GetValue2() - (maxHours * 100));

            uint16_t serverSum = (uint16_t)((clock.Hour * 60) + clock.Min);
            uint16_t minSum = (uint16_t)((minHours * 60) + minMinutes);
            uint16_t maxSum = (uint16_t)((maxHours * 60) + maxMinutes);

            if(maxSum < minSum)
            {
                // Compare, adjusting for day rollover (ex: 16:00-4:00)
                return serverSum >= minSum ||
                    (serverSum >= 1440 && (uint16_t)(serverSum - 1440) <= maxSum);
            }
            else
            {
                // Compare normally
                return minSum <= serverSum && serverSum <= maxSum;
            }
        }
    case objects::EventConditionData::Type_t::TIMESPAN_WEEK:
        if(compareMode != EventCompareMode::BETWEEN && compareMode != EventCompareMode::DEFAULT_COMPARE)
        {
            return false;
        }
        else
        {
            // System time between [value 1] and [value 2] (format: ddHHmm)
            // Days are represented as Sunday = 0, Monday = 1, etc
            // If 7 is specified for both days, any day is valid
            auto clock = mServer.lock()->GetWorldClockTime();

            int32_t val1 = condition->GetValue1();
            int32_t val2 = condition->GetValue2();

            int8_t minDays = (int8_t)floorl((float)val1 * 0.0001);
            int8_t minHours = (int8_t)floorl((float)(val1 - (minDays * 10000)) * 0.01);
            int8_t minMinutes = (int8_t)floorl((float)(val1 - (minDays * 10000)
                - (minHours * 100)) * 0.01);

            int8_t maxDays = (int8_t)floorl((float)val2 * 0.0001);
            int8_t maxHours = (int8_t)floorl((float)(val2 - (maxDays * 10000)) * 0.01);
            int8_t maxMinutes = (int8_t)floorl((float)(val2 - (maxDays * 10000)
                - (maxHours * 100)) * 0.01);

            bool skipDay = minDays == 7 && maxDays == 7;

            uint16_t systemSum = (uint16_t)(
                ((skipDay ? 0 : (clock.WeekDay - 1)) * 24 * 60 * 60) +
                (clock.SystemHour * 60) + clock.SystemMin);
            uint16_t minSum = (uint16_t)(((skipDay ? 0 : minDays) * 24 * 60 * 60) +
                (minHours * 60) + minMinutes);
            uint16_t maxSum = (uint16_t)(((skipDay ? 0 : maxDays) * 24 * 60 * 60) +
                (maxHours * 60) + maxMinutes);

            if(maxSum < minSum)
            {
                // Compare, adjusting for week rollover (ex: Friday through Sunday)
                return systemSum >= minSum || systemSum <= maxSum;
            }
            else
            {
                // Compare normally
                return minSum <= systemSum && systemSum <= maxSum;
            }
        }
    case objects::EventConditionData::Type_t::MOON_PHASE:
        {
            // Server moon phase = [value 1]
            auto clock = mServer.lock()->GetWorldClockTime();

            if(compareMode == EventCompareMode::BETWEEN)
            {
                // Compare, adjusting for week rollover (ex: 14 through 2)
                return clock.MoonPhase >= (int8_t)condition->GetValue1() ||
                    clock.MoonPhase <= (int8_t)condition->GetValue2();
            }
            else if(compareMode == EventCompareMode::EXISTS)
            {
                // Value is flag mask, check if the current phase is contained
                return ((condition->GetValue1() >> clock.MoonPhase) & 0x01) != 0;
            }
            else
            {
                return Compare((int32_t)clock.MoonPhase, condition->GetValue1(), 0,
                    compareMode, EventCompareMode::EQUAL, EVENT_COMPARE_NUMERIC);
            }
        }
    case objects::EventConditionData::Type_t::MAP:
        if(!client || (compareMode != EventCompareMode::EQUAL &&
            compareMode != EventCompareMode::DEFAULT_COMPARE))
        {
            return false;
        }
        else
        {
            // Map flag [value 1] = [value 2]
            auto character = client->GetClientState()->GetCharacterState()->GetEntity();
            auto progress = character->GetProgress().Get();

            uint16_t mapID = (uint16_t)condition->GetValue1();

            size_t index;
            uint8_t shiftVal;
            CharacterManager::ConvertIDToMaskValues(mapID, index, shiftVal);

            uint8_t indexVal = progress->GetMaps(index);

            return ((indexVal & shiftVal) == 0) == (condition->GetValue2() == 0);
        }
    case objects::EventConditionData::Type_t::QUEST_ACTIVE:
        if(!client || (compareMode != EventCompareMode::EQUAL &&
            compareMode != EventCompareMode::DEFAULT_COMPARE))
        {
            return false;
        }
        else
        {
            // Quest ID [value 1] active check = [value 2] (1 for active, 0 for not active)
            auto character = client->GetClientState()->GetCharacterState()->GetEntity();

            return character->GetQuests(
                (int16_t)condition->GetValue1()).IsNull() == (condition->GetValue2() == 0);
        }
    case objects::EventConditionData::Type_t::QUEST_SEQUENCE:
        if(!client || (compareMode != EventCompareMode::EQUAL &&
            compareMode != EventCompareMode::DEFAULT_COMPARE))
        {
            return false;
        }
        else
        {
            // Quest ID [value 1] is on its final phase (since this will progress the story)
            int16_t prevQuestID = (int16_t)condition->GetValue1();
            auto character = client->GetClientState()->GetCharacterState()->GetEntity();
            auto prevQuest = character->GetQuests(prevQuestID).Get();

            if(prevQuest == nullptr)
            {
                return false;
            }

            auto definitionManager = mServer.lock()->GetDefinitionManager();
            auto prevQuestData = definitionManager->GetQuestData((uint32_t)prevQuestID);

            if(prevQuestData == nullptr)
            {
                LOG_ERROR(libcomp::String("Invalid previous quest ID supplied for"
                    " EvaluateCondition: %1\n").Arg(prevQuestID));
                return false;
            }

            // Compare adjusting for zero index
            return prevQuestData->GetPhaseCount() == (uint32_t)(prevQuest->GetPhase() + 1);
        }
    case objects::EventConditionData::Type_t::EXPERTISE_NOT_MAX:
        if(!client || (compareMode != EventCompareMode::EQUAL &&
            compareMode != EventCompareMode::DEFAULT_COMPARE))
        {
            return false;
        }
        else if(condition->GetValue2() > 0)
        {
            // Ignore [value 1] and check if the number of points left to gain is greater than
            // [value 2]
            auto character = client->GetClientState()->GetCharacterState()->GetEntity();

            int32_t maxTotalPoints = mServer.lock()->GetCharacterManager()
                ->GetMaxExpertisePoints(character);

            int32_t currentPoints = 0;
            for(auto expertise : character->GetExpertises())
            {
                if(!expertise.IsNull())
                {
                    currentPoints = currentPoints + expertise->GetPoints();
                }
            }

            return Compare(condition->GetValue2(), maxTotalPoints - currentPoints, 0, compareMode,
                EventCompareMode::GTE, EVENT_COMPARE_NUMERIC);
        }
        else
        {
            // Expertise ID [value 1] is not maxed out
            auto definitionManager = mServer.lock()->GetDefinitionManager();
            auto expDef = definitionManager->GetExpertClassData((uint32_t)condition->GetValue1());
            if(expDef == nullptr)
            {
                LOG_ERROR(libcomp::String("Invalid expertise ID supplied for"
                    " EvaluateCondition: %1\n").Arg(condition->GetValue1()));
                return false;
            }

            auto character = client->GetClientState()->GetCharacterState()->GetEntity();
            auto exp = character->GetExpertises((size_t)condition->GetValue1()).Get();
            int32_t maxPoints = (expDef->GetMaxClass() * 100 * 1000)
                + (expDef->GetMaxRank() * 100 * 100);

            return exp == nullptr || (exp->GetPoints() < maxPoints);
        }
    case objects::EventConditionData::Type_t::EXPERTISE:
        if(!client)
        {
            return false;
        }
        else
        {
            // Expertise ID [value 1] compares to [value 2] (points or class check)
            auto character = client->GetClientState()->GetCharacterState()->GetEntity();
            auto exp = character->GetExpertises((size_t)condition->GetValue1()).Get();

            int32_t val = condition->GetValue2();
            int32_t compareTo = exp ? exp->GetPoints() : 0;
            if(val <= 10)
            {
                // Class check
                compareTo = (int32_t)floorl((float)compareTo * 0.00001f);
            }

            return Compare(compareTo, val, 0, compareMode, EventCompareMode::GTE,
                EVENT_COMPARE_NUMERIC);
        }
    case objects::EventConditionData::Type_t::SI_EQUIPPED:
        {
            LOG_ERROR("Currently unsupported SI_EQUIPPED condition encountered"
                " in EvaluateCondition\n");
            return false;
        }
    case objects::EventConditionData::Type_t::SUMMONED:
        if(!client)
        {
            return false;
        }
        else
        {
            // Partner demon of type [value 1] is currently summoned
            // If [value 2] = 1, the base demon type will be checked instead
            // Compare mode EXISTS ignores the type altogether
            auto dState = client->GetClientState()->GetDemonState();
            auto demon = dState->GetEntity();

            if(compareMode == EventCompareMode::EXISTS)
            {
                return demon != nullptr;
            }

            if(compareMode != EventCompareMode::EQUAL &&
                compareMode != EventCompareMode::DEFAULT_COMPARE)
            {
                return false;
            }

            if(demon)
            {
                if(condition->GetValue2() == 1)
                {
                    auto demonData = dState->GetDevilData();
                    return demonData && demonData->GetUnionData()->GetBaseDemonID() ==
                        (uint32_t)condition->GetValue1();
                }
                else
                {
                    return demon->GetType() == (uint32_t)condition->GetValue1();
                }
            }
            else
            {
                return false;
            }
        }
    // Custom conditions below this point
    case objects::EventCondition::Type_t::CLAN_HOME:
        if(!client || (compareMode != EventCompareMode::EQUAL &&
            compareMode != EventCompareMode::DEFAULT_COMPARE))
        {
            return false;
        }
        else
        {
            // Character homepoint zone = [value 1]
            auto character = client->GetClientState()->GetCharacterState()->GetEntity();

            return character->GetHomepointZone() == (uint32_t)condition->GetValue1();
        }
    case objects::EventConditionData::Type_t::COMP_DEMON:
        if(!client || (compareMode != EventCompareMode::EXISTS &&
            compareMode != EventCompareMode::DEFAULT_COMPARE))
        {
            return false;
        }
        else
        {
            // Demon of type [value 1] exists in the COMP
            auto character = client->GetClientState()->GetCharacterState()->GetEntity();
            auto progress = character->GetProgress();
            auto comp = character->GetCOMP().Get();

            std::set<uint32_t> demonIDs;
            size_t maxSlots = (size_t)progress->GetMaxCOMPSlots();
            for(size_t i = 0; i < maxSlots; i++)
            {
                auto slot = comp->GetDemons(i);
                if(!slot.IsNull())
                {
                    demonIDs.insert(slot->GetType());
                }
            }

            return demonIDs.find((uint32_t)condition->GetValue1()) !=
                demonIDs.end();
        }
    case objects::EventConditionData::Type_t::COMP_FREE:
        if(!client)
        {
            return false;
        }
        else
        {
            // COMP slots free compares to [value 1] (and [value 2])
            auto character = client->GetClientState()->GetCharacterState()->GetEntity();
            auto progress = character->GetProgress();
            auto comp = character->GetCOMP().Get();

            int32_t freeCount = 0;
            size_t maxSlots = (size_t)progress->GetMaxCOMPSlots();
            for(size_t i = 0; i < maxSlots; i++)
            {
                auto slot = comp->GetDemons(i);
                if(slot.IsNull())
                {
                    freeCount++;
                }
            }

            return Compare(freeCount, condition->GetValue1(), condition->GetValue2(),
                compareMode, EventCompareMode::EQUAL, EVENT_COMPARE_NUMERIC2);
        }
    case objects::EventCondition::Type_t::DEMON_BOOK:
        if(!client)
        {
            return false;
        }
        else if(compareMode == EventCompareMode::EXISTS)
        {
            // Demon ID ([value 2] = 0) or base demon ID ([value 2] != 0) matching [value 1]
            // exists in the compendium
            auto server = mServer.lock();
            auto definitionManager = server->GetDefinitionManager();

            auto character = client->GetClientState()->GetCharacterState()->GetEntity();
            auto progress = character->GetProgress();

            uint32_t demonType = (uint32_t)condition->GetValue1();
            bool baseMode = condition->GetValue2() != 0;

            for(auto dbPair : definitionManager->GetDevilBookData())
            {
                if((baseMode && dbPair.second->GetBaseID1() == demonType) ||
                    (!baseMode && dbPair.second->GetID() == demonType))
                {
                    size_t index;
                    uint8_t shiftValue;
                    CharacterManager::ConvertIDToMaskValues(
                        (uint16_t)dbPair.second->GetShiftValue(), index, shiftValue);
                    if((progress->GetDevilBook(index) & shiftValue) != 0)
                    {
                        return true;
                    }
                }
            }

            return false;
        }
        else
        {
            // Compendium entry count compares to [value 1] (and [value 2])
            auto dState = client->GetClientState()->GetDemonState();

            return Compare((int32_t)dState->GetCompendiumCount(), condition->GetValue1(),
                condition->GetValue2(), compareMode, EventCompareMode::GTE, EVENT_COMPARE_NUMERIC2);
        }
    case objects::EventCondition::Type_t::EXPERTISE_ACTIVE:
        if(!client || (compareMode != EventCompareMode::EQUAL &&
            compareMode != EventCompareMode::DEFAULT_COMPARE))
        {
            return false;
        }
        else
        {
            // Expertise ID [value 1] is active ([value 2] != 1) or locked ([value 2] = 1)
            auto character = client->GetClientState()->GetCharacterState()->GetEntity();

            auto exp = character->GetExpertises((size_t)condition->GetValue1()).Get();
            return condition->GetValue2() == 1 ? (!exp || exp->GetDisabled()) : (exp && !exp->GetDisabled());
        }
    case objects::EventCondition::Type_t::EQUIPPED:
        if(!client)
        {
            return false;
        }
        else
        {
            // Character has item type [value 1] equipped
            auto character = client->GetClientState()->GetCharacterState()->GetEntity();

            auto itemData = mServer.lock()->GetDefinitionManager()->GetItemData((uint32_t)condition->GetValue1());
            auto equip = itemData
                ? character->GetEquippedItems((size_t)itemData->GetBasic()->GetEquipType()).Get() : nullptr;
            return equip && equip->GetType() == (uint32_t)condition->GetValue1();
        }
    case objects::EventCondition::Type_t::GENDER:
        if(!client || (compareMode != EventCompareMode::EQUAL &&
            compareMode != EventCompareMode::DEFAULT_COMPARE))
        {
            return false;
        }
        else
        {
            // Character gender = [value 1]
            auto character = client->GetClientState()->GetCharacterState()->GetEntity();

            return (int32_t)character->GetGender() == condition->GetValue1();
        }
    case objects::EventCondition::Type_t::INSTANCE_ACCESS:
        if(!client)
        {
            return false;
        }
        else
        {
            // Character has access to instance of type compares to type [value 1]
            // or any belonging to the current zone if EXISTS
            auto instance = mServer.lock()->GetZoneManager()->GetInstanceAccess(client);

            if(compareMode == EventCompareMode::EXISTS)
            {
                if(!instance)
                {
                    return false;
                }

                auto zone = client->GetClientState()->GetCharacterState()->GetZone();
                auto currentInstance = zone->GetInstance();

                auto def = instance->GetDefinition();
                auto currentDef = currentInstance
                    ? currentInstance->GetDefinition() : nullptr;
                auto currentZoneDef = zone->GetDefinition();

                // true if the instance is the same, the lobby is the same or they
                // are in the lobby
                return instance == currentInstance ||
                    (currentDef && def->GetLobbyID() == currentDef->GetLobbyID()) ||
                    def->GetLobbyID() == currentZoneDef->GetID();
            }

            auto def = instance ? instance->GetDefinition() : nullptr;
            return Compare((int32_t)(def ? def->GetID() : 0), condition->GetValue1(),
                condition->GetValue2(), compareMode, EventCompareMode::EQUAL,
                EVENT_COMPARE_NUMERIC2);
        }
    case objects::EventConditionData::Type_t::INVENTORY_FREE:
        if(!client)
        {
            return false;
        }
        else
        {
            // Inventory slots free compares to [value 1] (and [value 2])
            // (does not acocunt for stacks that can be added to)
            auto character = client->GetClientState()->GetCharacterState()->GetEntity();
            auto inventory = character->GetItemBoxes(0);

            int32_t freeCount = 0;
            for(size_t i = 0; i < 50; i++)
            {
                auto item = inventory->GetItems(i);
                if(item.IsNull())
                {
                    freeCount++;
                }
            }

            return Compare(freeCount, condition->GetValue1(), condition->GetValue2(),
                compareMode, EventCompareMode::GTE, EVENT_COMPARE_NUMERIC2);
        }
    case objects::EventCondition::Type_t::LNC:
        if(!client)
        {
            return false;
        }
        else
        {
            // Character LNC points compares to [value 1] (and [value 2])
            auto character = client->GetClientState()->GetCharacterState()->GetEntity();

            return Compare((int32_t)character->GetLNC(), condition->GetValue1(),
                condition->GetValue2(), compareMode, EventCompareMode::BETWEEN,
                EVENT_COMPARE_NUMERIC2);
        }
    case objects::EventConditionData::Type_t::MATERIAL:
        if(!client)
        {
            return false;
        }
        else
        {
            // Material type [value 1] compares to [value 2]
            auto character = client->GetClientState()->GetCharacterState()->GetEntity();

            return Compare((int32_t)character->GetMaterials((uint32_t)condition->GetValue1()),
                condition->GetValue2(), 0, compareMode, EventCompareMode::GTE,
                EVENT_COMPARE_NUMERIC);
        }
    case objects::EventCondition::Type_t::NPC_STATE:
        if(!client)
        {
            return false;
        }
        else
        {
            // NPC in the same zone with actor ID [value 1] state compares to [value 2]
            auto npc = client->GetClientState()->GetCharacterState()->GetZone()
                ->GetActor(condition->GetValue1());

            uint8_t npcState = 0;
            if(!npc)
            {
                return false;
            }

            switch(npc->GetEntityType())
            {
            case EntityType_t::NPC:
                npcState = std::dynamic_pointer_cast<NPCState>(npc)
                    ->GetEntity()->GetState();
                break;
            case EntityType_t::OBJECT:
                npcState = std::dynamic_pointer_cast<ServerObjectState>(npc)
                    ->GetEntity()->GetState();
                break;
            default:
                return false;
            }

            return npc && Compare((int32_t)npcState, condition->GetValue2(),
                0, compareMode, EventCompareMode::EQUAL, EVENT_COMPARE_NUMERIC);
        }
    case objects::EventCondition::Type_t::PARTY_SIZE:
        if(!client)
        {
            return false;
        }
        else
        {
            // Party size compares to [value 1] (and [value 2])
            // (no party counts as 0, not 1)
            auto party = client->GetClientState()->GetParty();
            if(compareMode == EventCompareMode::EXISTS)
            {
                return party != nullptr;
            }

            return Compare((int32_t)(party ? party->MemberIDsCount() : 0),
                condition->GetValue1(), condition->GetValue2(), compareMode,
                EventCompareMode::BETWEEN, EVENT_COMPARE_NUMERIC2);
        }
    case objects::EventCondition::Type_t::PLUGIN:
        if(!client || (compareMode != EventCompareMode::EQUAL &&
            compareMode != EventCompareMode::DEFAULT_COMPARE))
        {
            return false;
        }
        else
        {
            // Plugin flag [value 1] = [value 2]
            auto character = client->GetClientState()->GetCharacterState()->GetEntity();
            auto progress = character->GetProgress().Get();

            uint16_t pluginID = (uint16_t)condition->GetValue1();

            size_t index;
            uint8_t shiftVal;
            CharacterManager::ConvertIDToMaskValues(pluginID, index, shiftVal);

            uint8_t indexVal = progress->GetPlugins(index);

            return ((indexVal & shiftVal) == 0) == (condition->GetValue2() == 0);
        }
    case objects::EventCondition::Type_t::SKILL_LEARNED:
        if(!client)
        {
            return false;
        }
        else
        {
            // Character currently knows skill with ID [value 1]
            return (compareMode == EventCompareMode::EQUAL ||
                compareMode == EventCompareMode::DEFAULT_COMPARE) &&
                client->GetClientState()->GetCharacterState()->CurrentSkillsContains(
                    (uint32_t)condition->GetValue1());
        }
    case objects::EventCondition::Type_t::STAT_VALUE:
        if(!client)
        {
            return false;
        }
        else
        {
            // Character stat at correct index [value 1] compares to [value 2]
            return Compare((int32_t)client->GetClientState()->GetCharacterState()->GetCorrectValue(
                (CorrectTbl)condition->GetValue1()), condition->GetValue2(), 0,
                compareMode, EventCompareMode::GTE, EVENT_COMPARE_NUMERIC);
        }
    case objects::EventCondition::Type_t::STATUS_ACTIVE:
        if(!client || (compareMode != EventCompareMode::EXISTS &&
            compareMode != EventCompareMode::DEFAULT_COMPARE))
        {
            return false;
        }
        else
        {
            // Character ([value 2] = 0) or demon ([value 2] != 0) has status effect [value 1]
            auto eState = condition->GetValue2() == 0
                ? std::dynamic_pointer_cast<ActiveEntityState>(
                    client->GetClientState()->GetCharacterState())
                : std::dynamic_pointer_cast<ActiveEntityState>(
                    client->GetClientState()->GetDemonState());

            auto statusEffects = eState->GetStatusEffects();
            return statusEffects.find((uint32_t)condition->GetValue1()) != statusEffects.end();
        }
    case objects::EventCondition::Type_t::TIMESPAN_DATETIME:
        if(compareMode != EventCompareMode::BETWEEN && compareMode != EventCompareMode::DEFAULT_COMPARE)
        {
            return false;
        }
        else
        {
            // System time between [value 1] and [value 2] (format: MMddHHmm)
            // Month is represented as January = 1, February = 2, etc
            auto clock = mServer.lock()->GetWorldClockTime();

            int32_t minVal = condition->GetValue1();
            int32_t maxVal = condition->GetValue2();

            int32_t systemSum = (clock.Month * 1000000) +
                (clock.Day * 10000) + (clock.SystemHour * 100) +
                clock.SystemMin;

            if(maxVal < minVal)
            {
                // Compare, adjusting for year rollover (ex: Dec 31st to Jan 1st)
                return systemSum >= minVal || systemSum <= maxVal;
            }
            else
            {
                // Compare normally
                return minVal <= systemSum && systemSum <= maxVal;
            }
        }
    case objects::EventCondition::Type_t::QUESTS_ACTIVE:
        if(!client)
        {
            return false;
        }
        else
        {
            // Active quest count compares to [value 1] (and [value 2])
            auto character = client->GetClientState()->GetCharacterState()->GetEntity();

            return Compare((int32_t)character->QuestsCount(), condition->GetValue1(),
                condition->GetValue2(), compareMode, EventCompareMode::EQUAL,
                EVENT_COMPARE_NUMERIC2);
        }
    case objects::EventConditionData::Type_t::NONE:
    default:
        LOG_ERROR(libcomp::String("Invalid condition type supplied for"
            " EvaluateCondition: %1\n").Arg((uint32_t)condition->GetType()));
        break;
    }

    return false;
}

bool EventManager::EvaluateQuestPhaseRequirements(const std::shared_ptr<
    ChannelClientConnection>& client, int16_t questID, int8_t phase)
{
    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto questData = definitionManager->GetQuestData((uint32_t)questID);
    if(!questData)
    {
        LOG_ERROR(libcomp::String("Invalid quest ID supplied for"
            " EvaluateQuestPhaseRequirements: %1\n").Arg(questID));
        return false;
    }

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto quest = character->GetQuests(questID).Get();

    int8_t currentPhase = quest ? quest->GetPhase() : -1;
    if(currentPhase < 0 || currentPhase != phase ||
        (int8_t)questData->GetPhaseCount() < currentPhase)
    {
        return false;
    }

    // If any requirement does not pass, return false
    auto phaseData = questData->GetPhases((size_t)currentPhase);
    for(uint32_t i = 0; i < phaseData->GetRequirementCount(); i++)
    {
        auto req = phaseData->GetRequirements((size_t)i);
        switch(req->GetType())
        {
        case objects::QuestPhaseRequirement::Type_t::ITEM:
            {
                uint32_t count = server->GetCharacterManager()
                    ->GetExistingItemCount(character, req->GetObjectID());
                if(count < req->GetObjectCount())
                {
                    return false;
                }
            }
            break;
        case objects::QuestPhaseRequirement::Type_t::SUMMON:
            {
                auto dState = state->GetDemonState();
                auto demon = dState->GetEntity();

                if(demon == nullptr || demon->GetType() != req->GetObjectID())
                {
                    return false;
                }
            }
            break;
        case objects::QuestPhaseRequirement::Type_t::KILL:
        case objects::QuestPhaseRequirement::Type_t::KILL_HIDDEN:
            {
                int32_t customData = quest->GetCustomData((size_t)i);
                if(customData < (int32_t)req->GetObjectCount())
                {
                    return false;
                }
            }
            break;
        case objects::QuestPhaseRequirement::Type_t::NONE:
        default:
            LOG_ERROR(libcomp::String("Invalid requirement type encountered for"
                " EvaluateQuestPhaseRequirements in quest '%1' phase '%2': %3\n")
                .Arg(questID).Arg(currentPhase).Arg((uint32_t)req->GetType()));
            return false;
        }
    }

    return true;
}

void EventManager::UpdateQuestTargetEnemies(
    const std::shared_ptr<ChannelClientConnection>& client)
{
    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    // Clear existing
    state->ClearQuestTargetEnemies();

    // Re-calculate targets
    for(auto qPair : character->GetQuests())
    {
        auto quest = qPair.second.Get();
        auto questData = definitionManager->GetQuestData((uint32_t)qPair.first);
        int8_t currentPhase = quest ? quest->GetPhase() : -1;
        if(currentPhase < 0 || (int8_t)questData->GetPhaseCount() < currentPhase)
        {
            continue;
        }

        auto phaseData = questData->GetPhases((size_t)currentPhase);
        for(uint32_t i = 0; i < phaseData->GetRequirementCount(); i++)
        {
            auto req = phaseData->GetRequirements((size_t)i);
            if(req->GetType() == objects::QuestPhaseRequirement::Type_t::KILL_HIDDEN ||
                req->GetType() == objects::QuestPhaseRequirement::Type_t::KILL)
            {
                state->InsertQuestTargetEnemies(req->GetObjectID());
            }
        }
    }

    // Add demon quest type
    auto dQuest = character->GetDemonQuest().Get();
    if(dQuest)
    {
        for(auto& targetPair : dQuest->GetTargets())
        {
            state->InsertQuestTargetEnemies(targetPair.first);
        }
    }
}

void EventManager::SendActiveQuestList(
    const std::shared_ptr<ChannelClientConnection>& client)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto questMap = character->GetQuests();

    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_QUEST_ACTIVE_LIST);

    reply.WriteS8((int8_t)questMap.size());
    for(auto kv : questMap)
    {
        auto quest = kv.second;
        auto customData = quest->GetCustomData();

        reply.WriteS16Little(quest->GetQuestID());
        reply.WriteS8(quest->GetPhase());

        reply.WriteArray(&customData, (uint32_t)customData.size() * sizeof(int32_t));
    }

    client->SendPacket(reply);
}

void EventManager::SendCompletedQuestList(
    const std::shared_ptr<ChannelClientConnection>& client)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto completedQuests = character->GetProgress()->GetCompletedQuests();
    
    libcomp::Packet reply;
    reply.WritePacketCode(ChannelToClientPacketCode_t::PACKET_QUEST_COMPLETED_LIST);
    reply.WriteU16Little((uint16_t)completedQuests.size());
    reply.WriteArray(&completedQuests, (uint32_t)completedQuests.size());

    client->SendPacket(reply);
}

std::shared_ptr<objects::DemonQuest> EventManager::GenerateDemonQuest(
    const std::shared_ptr<CharacterState>& cState,
    const std::shared_ptr<objects::Demon>& demon)
{
    auto character = cState->GetEntity();

    if(!demon || !demon->GetHasQuest() || !character->GetDemonQuest().IsNull())
    {
        return nullptr;
    }

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto serverDataManager = server->GetServerDataManager();

    // Generate the pending quest but don't save it until its accepted
    auto dQuest = libcomp::PersistentObject::New<objects::DemonQuest>();

    dQuest->SetDemon(demon->GetUUID());
    dQuest->SetCharacter(character->GetUUID());

    uint8_t lvl = (uint8_t)demon->GetCoreStats()->GetLevel();
    auto demonData = definitionManager->GetDevilData(demon->GetType());
    uint8_t raceID = (uint8_t)demonData->GetCategory()->GetRace();

    // Gather the valid types based the requesting demon
    uint16_t enabledTypeFlags = server->GetWorldSharedConfig()
        ->GetEnabledDemonQuests();

    const uint8_t flagCount = (uint8_t)objects::DemonQuest::Type_t::PLASMA -
        (uint8_t)objects::DemonQuest::Type_t::KILL + 1;

    std::set<uint16_t> enabledTypes;
    for(uint8_t shift = 0; shift < flagCount; shift++)
    {
        if((enabledTypeFlags & (uint16_t)(0x0001 << shift)) != 0)
        {
            enabledTypes.insert((uint16_t)(shift + 1));
        }
    }

    // Default to enabled types
    std::set<uint16_t> validTypes = enabledTypes;

    // Remove conditional types to add back later
    validTypes.erase((uint16_t)objects::DemonQuest::Type_t::CRYSTALLIZE);
    validTypes.erase((uint16_t)objects::DemonQuest::Type_t::ENCHANT_TAROT);
    validTypes.erase((uint16_t)objects::DemonQuest::Type_t::ENCHANT_SOUL);
    validTypes.erase((uint16_t)objects::DemonQuest::Type_t::SYNTH_MELEE);
    validTypes.erase((uint16_t)objects::DemonQuest::Type_t::SYNTH_GUN);

    std::set<uint32_t> demonTraits;

    auto growth = demonData->GetGrowth();
    for(size_t i = 0; i < 4; i++)
    {
        uint32_t traitID = growth->GetTraits(i);
        if(traitID)
        {
            demonTraits.insert(traitID);
        }
    }

    uint8_t ssRank = cState->GetExpertiseRank(definitionManager,
        EXPERTISE_CHAIN_SWORDSMITH);
    uint8_t amRank = cState->GetExpertiseRank(definitionManager,
        EXPERTISE_CHAIN_ARMS_MAKER);

    // Synth based quests require a skill on that demon that boosts
    // the success
    for(auto pair : SVR_CONST.SYNTH_ADJUSTMENTS)
    {
        if(demonTraits.find((uint32_t)pair.first) != demonTraits.end())
        {
            switch(pair.second[0])
            {
            case 1:
                // Add back synth skills
                if(enabledTypes.find((uint16_t)objects::DemonQuest::Type_t::
                    CRYSTALLIZE) != enabledTypes.end())
                {
                    validTypes.insert((uint16_t)
                        objects::DemonQuest::Type_t::CRYSTALLIZE);
                }

                if(enabledTypes.find((uint16_t)objects::DemonQuest::Type_t::
                    ENCHANT_TAROT) != enabledTypes.end())
                {
                    validTypes.insert((uint16_t)
                        objects::DemonQuest::Type_t::ENCHANT_TAROT);
                }

                if(enabledTypes.find((uint16_t)objects::DemonQuest::Type_t::
                    ENCHANT_SOUL) != enabledTypes.end())
                {
                    validTypes.insert((uint16_t)
                        objects::DemonQuest::Type_t::ENCHANT_SOUL);
                }
                break;
            case 2:
                // Add melee synth if class 1 or higher
                if(enabledTypes.find((uint16_t)objects::DemonQuest::Type_t::
                    SYNTH_MELEE) != enabledTypes.end() && ssRank >= 10)
                {
                    validTypes.insert((uint16_t)
                        objects::DemonQuest::Type_t::SYNTH_MELEE);
                }
                break;
            case 3:
                // Add gun synth if class 1 or higher
                if(enabledTypes.find((uint16_t)objects::DemonQuest::Type_t::
                    SYNTH_GUN) != enabledTypes.end() && amRank >= 10)
                {
                    validTypes.insert((uint16_t)
                        objects::DemonQuest::Type_t::SYNTH_GUN);
                }
                break;
            default:
                return nullptr;
            }
        }
    }

    // Remove conditionally invalid types
    std::list<std::shared_ptr<objects::Item>> equipment;
    for(auto item : character->GetItemBoxes(0)->GetItems())
    {
        if(!item.IsNull())
        {
            auto itemData = definitionManager->GetItemData(item->GetType());
            if(!itemData) continue;

            switch(itemData->GetBasic()->GetEquipType())
            {
            case objects::MiItemBasicData::EquipType_t::EQUIP_TYPE_WEAPON:
                equipment.push_back(item.Get());
                break;
            default:
                /// @todo: enable armor too
                break;
            }
        }
    }

    if(validTypes.find((uint16_t)objects::DemonQuest::Type_t::
        EQUIPMENT_MOD) != validTypes.end() && equipment.size() == 0)
    {
        validTypes.erase((uint16_t)objects::DemonQuest::Type_t::
            EQUIPMENT_MOD);
    }

    // Randomly pick a valid type
    if(validTypes.size() > 0)
    {
        uint16_t typeID = libcomp::Randomizer::GetEntry(validTypes);
        dQuest->SetType((objects::DemonQuest::Type_t)typeID);
    }
    else
    {
        LOG_ERROR(libcomp::String("No valid demon quest could be generated"
            " for demon type '%1' on character: %2\n").Arg(demon->GetType())
            .Arg(character->GetUUID().ToString()));
        return nullptr;
    }

    // Now Build the quest

    // Specific quest types require that a demon can be obtained so they
    // are not impossible on the current server
    bool demonDependent = false;

    std::set<uint32_t> demons;
    switch(dQuest->GetType())
    {
    case objects::DemonQuest::Type_t::KILL:
    case objects::DemonQuest::Type_t::CONTRACT:
    case objects::DemonQuest::Type_t::CRYSTALLIZE:
    case objects::DemonQuest::Type_t::ENCHANT_TAROT:
    case objects::DemonQuest::Type_t::ENCHANT_SOUL:
        {
            bool isKill = dQuest->GetType() ==
                objects::DemonQuest::Type_t::KILL;
            int8_t cLevel = character->GetCoreStats()->GetLevel();

            std::map<int8_t, std::set<uint32_t>> fieldEnemyMap;
            for(auto& pair : serverDataManager->GetFieldZoneIDs())
            {
                auto zone = server->GetZoneManager()->GetGlobalZone(
                    pair.first, pair.second);
                if(zone)
                {
                    for(auto& spawnPair : zone->GetDefinition()
                        ->GetSpawns())
                    {
                        auto spawn = spawnPair.second;
                        bool canJoin = spawn->GetTalkResist() < 100 &&
                            (spawn->GetTalkResults() & 0x01) != 0 &&
                            spawn->GetLevel() <= cLevel;
                        if(spawn->GetLevel() && (isKill || canJoin))
                        {
                            fieldEnemyMap[spawn->GetLevel()].insert(
                                spawn->GetEnemyType());
                        }
                    }
                }
            }

            // Only keep levels within a range of +-10
            uint8_t lvlMax = (uint8_t)fieldEnemyMap.rbegin()->first;
            uint8_t lvlAdjust = lvl > lvlMax ? lvlMax : lvl;
            for(auto& pair : fieldEnemyMap)
            {
                if(abs(pair.first - lvlAdjust) <= 10)
                {
                    for(uint32_t enemyType : pair.second)
                    {
                        // Exclude demons of the same type if kill quest
                        if(!isKill || definitionManager->GetDevilData(
                            enemyType)->GetUnionData()->GetBaseDemonID() !=
                            demonData->GetUnionData()->GetBaseDemonID())
                        {
                            demons.insert(enemyType);
                        }
                    }
                }
            }

            demonDependent = true;
        }
        break;
    default:
        break;
    }

    // If type is an enchantment request, convert to base demon IDs and
    // only include ones with a valid enchantment entry
    switch(dQuest->GetType())
    {
    case objects::DemonQuest::Type_t::CRYSTALLIZE:
    case objects::DemonQuest::Type_t::ENCHANT_TAROT:
    case objects::DemonQuest::Type_t::ENCHANT_SOUL:
        {
            std::set<uint32_t> enchantDemons;

            // Include demons in the COMP (excluding the requestor)
            for(auto d : character->GetCOMP()->GetDemons())
            {
                if(!d.IsNull() && d.Get() != demon)
                {
                    demons.insert(d->GetType());
                }
            }

            for(uint32_t demonType : demons)
            {
                auto def = definitionManager->GetDevilData(demonType);
                if(def)
                {
                    uint32_t baseID = def->GetUnionData()->GetBaseDemonID();
                    auto enchantData = definitionManager
                        ->GetEnchantDataByDemonID(baseID);
                    if(enchantData)
                    {
                        enchantDemons.insert(baseID);
                    }
                }
            }

            // Never include the demon itself
            enchantDemons.erase(demon->GetType());

            demons = enchantDemons;
        }
        break;
    default:
        break;
    }

    // If an enemy is needed but none exist, switch to a different type
    if(demonDependent && demons.size() == 0)
    {
        // Default to the only one that is always (technically) possible
        dQuest->SetType(objects::DemonQuest::Type_t::ITEM);
    }

    switch(dQuest->GetType())
    {
    case objects::DemonQuest::Type_t::KILL:
        // Kill a randomly chosen field demon
        {
            int32_t lvlAdjust = (int32_t)ceil((float)lvl / 30.f);
            uint16_t left = RNG(uint16_t, 1, (uint16_t)(lvlAdjust + 4));

            // Chance to split larger groupings into multiple target types
            std::list<uint16_t> counts;
            if(left > 3 && RNG(int32_t, 1, lvlAdjust + 2) != 1)
            {
                while(left)
                {
                    uint16_t count = RNG(uint16_t,
                        counts.size() > 0 ? 1 : 2, left);
                    counts.push_back(count);

                    left = (uint16_t)(left - count);
                }
            }
            else
            {
                counts.push_back(left);
            }

            for(uint16_t count : counts)
            {
                uint32_t enemyType = libcomp::Randomizer::GetEntry(
                    demons);
                if(enemyType)
                {
                    demons.erase(enemyType);
                    dQuest->SetTargets(enemyType, count);
                }
                else
                {
                    // None left
                    break;
                }
            }
        }
        break;
    case objects::DemonQuest::Type_t::CONTRACT:
        // Contract a randomly chosen field demon
        dQuest->SetTargets(libcomp::Randomizer::GetEntry(demons), 1);
        break;
    case objects::DemonQuest::Type_t::FUSE:
        // Demon from fusion ranges of a random race (closest level)
        {
            uint8_t fuseRace = FUSION_RACE_MAP[0][RNG(uint16_t, 0, 33)];

            auto fRange = definitionManager->GetFusionRanges(fuseRace);

            std::pair<uint8_t, uint32_t> result(0, 0);
            for(auto& pair : fRange)
            {
                if(!result.first ||
                    abs(lvl - pair.first) < abs(lvl - result.first))
                {
                    result = pair;
                }
            }

            // Use found demon or default to self if none was found
            dQuest->SetTargets(result.second ? result.second
                : demonData->GetUnionData()->GetBaseDemonID(), 1);
        }
        break;
    case objects::DemonQuest::Type_t::ITEM:
        // Random amount of race bound crystals
        {
            // Default to magnetite just in case nothing matches
            uint32_t itemType = SVR_CONST.ITEM_MAGNETITE;
            for(auto& pair : SVR_CONST.DEMON_CRYSTALS)
            {
                if(pair.second.find(raceID) != pair.second.end())
                {
                    itemType = pair.first;
                    break;
                }
            }

            int32_t lvlAdjust = (int32_t)ceil((float)lvl / 20.f);
            dQuest->SetTargets(itemType,
                RNG(int32_t, lvlAdjust + 1, lvlAdjust + 3));
        }
        break;
    case objects::DemonQuest::Type_t::CRYSTALLIZE:
        // Random crystal from a specific demon
        {
            auto enchantData = definitionManager->GetEnchantDataByDemonID(
                libcomp::Randomizer::GetEntry(demons));
            if(enchantData)
            {
                dQuest->SetTargets(enchantData->GetDevilCrystal()
                    ->GetItemID(), 1);
            }
        }
        break;
    case objects::DemonQuest::Type_t::ENCHANT_TAROT:
    case objects::DemonQuest::Type_t::ENCHANT_SOUL:
        // Random crystal from a specific demon
        {
            auto enchantData = definitionManager->GetEnchantDataByDemonID(
                libcomp::Randomizer::GetEntry(demons));
            if(enchantData)
            {
                dQuest->SetTargets((uint32_t)enchantData->GetID(), 1);
            }
        }
        break;
    case objects::DemonQuest::Type_t::EQUIPMENT_MOD:
        // Random equipment modification based on the player's inventory
        {
            auto equip = libcomp::Randomizer::GetEntry(equipment);

            // Remove unslotted at lower levels
            if(lvl < 30)
            {
                equipment.remove_if([](
                    const std::shared_ptr<objects::Item>& item)
                    {
                        return item->GetModSlots(0) == 0;
                    });

                if(equipment.size() > 0)
                {
                    equip = libcomp::Randomizer::GetEntry(equipment);
                }
            }

            dQuest->SetTargets(equip->GetType(), 1);
        }
        break;
    case objects::DemonQuest::Type_t::SYNTH_MELEE:
    case objects::DemonQuest::Type_t::SYNTH_GUN:
        // Random synth result of the specific type
        {
            bool isSS = dQuest->GetType() ==
                objects::DemonQuest::Type_t::SYNTH_MELEE;

            std::list<std::shared_ptr<objects::MiSynthesisData>> synthList;
            for(auto pair : definitionManager->GetAllSynthesisData())
            {
                uint32_t skillID = pair.second->GetBaseSkillID();
                if((isSS && skillID == SVR_CONST.SYNTH_SKILLS[3]) ||
                    (!isSS && skillID == SVR_CONST.SYNTH_SKILLS[4]))
                {
                    synthList.push_back(pair.second);
                }
            }

            auto synth = libcomp::Randomizer::GetEntry(synthList);
            if(synth)
            {
                dQuest->SetTargets(synth->GetItemID(), 1);
            }
            else
            {
                LOG_ERROR("Failed to retrieve synth result for demon quest\n");
                return nullptr;
            }
        }
        break;
    case objects::DemonQuest::Type_t::PLASMA:
        // Random color, count between 10 and 30
        {
            // "Harder" colors show up more at higher levels
            int32_t lvlAdjust = (int32_t)floor((float)lvl / 10.f);
            uint32_t min = (uint32_t)(15 + lvlAdjust);    // Max 24
            uint32_t max = (uint32_t)(29 + lvlAdjust);    // Max 38
            dQuest->SetTargets((uint32_t)floor((float)
                RNG(uint32_t, min, max) / 10.f), RNG(int32_t, 10, 30));
        }
        break;
    default:
        return nullptr;
    }

    AddDemonQuestRewards(cState, demon, dQuest);

    return dQuest;
}

void EventManager::AddDemonQuestRewards(
    const std::shared_ptr<CharacterState>& cState,
    const std::shared_ptr<objects::Demon>& demon,
    std::shared_ptr<objects::DemonQuest>& dQuest)
{
    auto character = cState->GetEntity();
    auto progress = character->GetProgress().Get();

    auto server = mServer.lock();
    auto characterManager = server->GetCharacterManager();
    auto definitionManager = server->GetDefinitionManager();
    auto serverDataManager = server->GetServerDataManager();

    uint8_t lvl = (uint8_t)demon->GetCoreStats()->GetLevel();
    auto demonData = definitionManager->GetDevilData(demon->GetType());
    uint8_t raceID = (uint8_t)demonData->GetCategory()->GetRace();
    uint16_t familiarity = demon->GetFamiliarity();

    uint32_t nextSeq = (uint32_t)(progress->GetDemonQuestSequence() + 1);
    uint32_t nextRaceSeq = (uint32_t)(
        progress->GetDemonQuestsCompleted(raceID) + 1);

    std::unordered_map<uint32_t, std::list<std::shared_ptr<
        objects::DemonQuestReward>>> rewardGroups;
    for(auto& pair : serverDataManager->GetDemonQuestRewardData())
    {
        auto reward = pair.second;

        // Ignore invalid quest types
        if(reward->QuestTypesCount() &&
            !reward->QuestTypesContains((int8_t)dQuest->GetType()))
        {
            continue;
        }

        // Ignore invalid race
        if(reward->GetRaceID() && reward->GetRaceID() != raceID)
        {
            continue;
        }

        // Ignore invalid level range
        if(reward->GetLevelMin() > (uint8_t)lvl ||
            reward->GetLevelMax() < (uint8_t)lvl)
        {
            continue;
        }

        // Ignore invalid familiarity range
        if(reward->GetFamiliarityMin() > familiarity ||
            reward->GetFamiliarityMax() < familiarity)
        {
            continue;
        }

        // Ignore invalid sequence
        if(reward->GetSequenceStart())
        {
            uint32_t start = reward->GetSequenceStart();
            uint32_t repeat = reward->GetSequenceRepeat();
            uint32_t end = reward->GetSequenceEnd();

            uint32_t seq = reward->GetRaceID() ? nextRaceSeq : nextSeq;
            if(seq < start || (end && seq >= end) ||
                (!repeat && seq != start) ||
                (repeat && (seq - start) % repeat != 0))
            {
                continue;
            }
        }

        rewardGroups[reward->GetGroupID()].push_back(reward);
    }

    bool addPresent = false;
    std::set<uint32_t> chanceDropSets;
    for(auto& pair : rewardGroups)
    {
        auto rewards = pair.second;

        // Sort by ID
        rewards.sort([](
            const std::shared_ptr<objects::DemonQuestReward>& a,
            const std::shared_ptr<objects::DemonQuestReward>& b)
            {
                return a->GetID() < b->GetID();
            });

        if(pair.first != 0 && rewards.size() > 1)
        {
            // Only apply the last one for grouped rewards
            std::list<std::shared_ptr<objects::DemonQuestReward>> temp;
            temp.push_back(rewards.back());
            temp = rewards;
        }

        // Add rewards (do not sum item stacks)
        for(auto& reward : rewards)
        {
            bool added = false;

            for(uint32_t dropSetID : reward->GetNormalDropSets())
            {
                // Check drop rate for all items being added
                auto dropSet = serverDataManager->GetDropSetData(
                    dropSetID);
                if(!dropSet) continue;

                for(auto drop : characterManager->DetermineDrops(
                    dropSet->GetDrops(), 0))
                {
                    dQuest->SetRewardItems(drop->GetItemType(),
                        RNG(uint16_t, drop->GetMinStack(),
                            drop->GetMaxStack()));
                }

                added = true;
            }

            // Ignore titles if the player already has them
            std::list<uint16_t> newTitles;
            for(uint16_t title : reward->GetBonusTitles())
            {
                size_t index;
                uint8_t shiftVal;
                CharacterManager::ConvertIDToMaskValues(title, index,
                    shiftVal);

                uint8_t indexVal = progress->GetSpecialTitles(index);
                if((shiftVal & indexVal) == 0)
                {
                    newTitles.push_back(title);
                }
            }

            bool take1 = reward->GetBonusMode() ==
                objects::DemonQuestReward::BonusMode_t::SINGLE;

            if(reward->BonusDropSetsCount())
            {
                // Filter by drops by rate
                std::list<std::shared_ptr<objects::ItemDrop>> drops;
                for(uint32_t dropSetID : reward->GetBonusDropSets())
                {
                    auto dropSet = serverDataManager->GetDropSetData(
                        dropSetID);
                    if(!dropSet) continue;

                    for(auto drop : characterManager->DetermineDrops(
                        dropSet->GetDrops(), 0))
                    {
                        drops.push_back(drop);
                    }
                }

                if(take1 && drops.size() > 1)
                {
                    // Randomly select one
                    std::list<std::shared_ptr<objects::ItemDrop>> temp;
                    temp.push_back(libcomp::Randomizer::GetEntry(drops));
                    drops = temp;
                }

                for(auto& drop : drops)
                {
                    dQuest->SetBonusItems(drop->GetItemType(),
                        RNG(uint16_t, drop->GetMinStack(),
                            drop->GetMaxStack()));
                }

                added = true;
            }

            if(newTitles.size() > 0)
            {
                if(take1 && newTitles.size() > 1)
                {
                    // Take the first one
                    std::list<uint16_t> temp;
                    temp.push_back(newTitles.front());
                    newTitles = temp;
                }

                for(uint16_t title : newTitles)
                {
                    dQuest->AppendBonusTitles(title);
                }

                added = true;
            }

            if(reward->GetBonusXP() > 0)
            {
                dQuest->AppendBonusXP(reward->GetBonusXP());
                added = true;
            }

            if(reward->ChanceDropSetsCount() > 0)
            {
                for(uint32_t dropSetID : reward->GetChanceDropSets())
                {
                    chanceDropSets.insert(dropSetID);
                }

                added = true;
            }

            // If no items or bonuses were valid, default to one
            // item from the demon present set
            addPresent |= !added;
        }
    }

    if(addPresent)
    {
        // Add one demon present item
        int8_t rarity = 0;
        uint32_t presentType = characterManager->GetDemonPresent(
            demon->GetType(), (int8_t)lvl, familiarity, rarity);
        if(presentType && !dQuest->BonusItemsKeyExists(presentType))
        {
            dQuest->SetBonusItems(presentType, 1);
        }
    }

    // Calculate normal XP gain
    int8_t cLvl = character->GetCoreStats()->GetLevel();
    if(cLvl < 99)
    {
        // Formula estimated from collected data, not 100% accurate
        double lvlXP = (double)libcomp::LEVEL_XP_REQUIREMENTS[(size_t)cLvl];
        double normalXP = floor(((0.00000691775 * (double)(cLvl * cLvl)) -
            (0.001384 * (double)cLvl) + 0.06922) * lvlXP);

        dQuest->SetXPReward((int32_t)normalXP);
    }

    // Calculate sequential XP gain
    uint16_t idx = 0;
    for(auto iter = SVR_CONST.DEMON_QUEST_XP.begin();
        iter != SVR_CONST.DEMON_QUEST_XP.end(); iter++)
    {
        if(nextSeq == 5 && idx == 0)
        {
            // Reward at 5
            dQuest->AppendBonusXP(*iter);
            break;
        }
        else
        {
            bool onFinal = (size_t)(idx + 1) ==
                SVR_CONST.DEMON_QUEST_XP.size();
            if(nextSeq < 100 && nextSeq % 10 == 0)
            {
                // Reward every 10 <= 100
                if(onFinal || (idx == (uint16_t)(nextSeq / 10)))
                {
                    dQuest->AppendBonusXP(*iter);
                    break;
                }
            }
            else if (nextSeq >= 100 && nextSeq % 50 == 0)
            {
                // Reward every 50 >= 100
                if(onFinal || (idx == (uint16_t)(nextSeq / 50)))
                {
                    dQuest->AppendBonusXP(*iter);
                    break;
                }
            }
        }

        idx++;
    }

    if(chanceDropSets.size() > 0)
    {
        // Set one random chance item
        std::list<std::shared_ptr<objects::ItemDrop>> drops;
        for(uint32_t dropSetID : chanceDropSets)
        {
            auto dropSet = serverDataManager->GetDropSetData(dropSetID);
            if(!dropSet) continue;

            for(auto drop : characterManager->DetermineDrops(
                dropSet->GetDrops(), 0))
            {
                drops.push_back(drop);
            }
        }

        auto drop = libcomp::Randomizer::GetEntry(drops);
        dQuest->SetChanceItem(drop->GetItemType());
        dQuest->SetChanceItemCount(RNG(uint16_t,
            drop->GetMinStack(), drop->GetMaxStack()));
    }
}

bool EventManager::UpdateDemonQuestCount(
    const std::shared_ptr<ChannelClientConnection>& client,
    objects::DemonQuest::Type_t questType, uint32_t targetType,
    int32_t increment)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto dQuest = character->GetDemonQuest().Get();

    bool itemMode = questType == objects::DemonQuest::Type_t::ITEM;
    if(dQuest && dQuest->GetType() == questType &&
        (dQuest->TargetsKeyExists(targetType) || (!targetType && itemMode)))
    {
        bool updated = false;

        auto server = mServer.lock();
        for(auto& targetPair : dQuest->GetTargets())
        {
            if(targetType && targetType != targetPair.first) continue;

            int32_t newCount = 0;
            int32_t currentCount = dQuest->GetTargetCurrentCounts(
                targetPair.first);
            if(itemMode)
            {
                // Ignore increment, set to current
                newCount = (int32_t)server->GetCharacterManager()
                    ->GetExistingItemCount(character, targetPair.first);
            }
            else
            {
                // Increment by the supplied amount
                newCount = increment + currentCount;
            }

            // Do not exceed required amount
            if(newCount > targetPair.second)
            {
                newCount = targetPair.second;
            }

            // If new count differs, update and send to client
            if(newCount != currentCount)
            {
                dQuest->SetTargetCurrentCounts(targetPair.first, newCount);

                libcomp::Packet p;
                p.WritePacketCode(ChannelToClientPacketCode_t::
                    PACKET_DEMON_QUEST_COUNT_UPDATE);
                p.WriteU32Little(targetPair.first);
                p.WriteS32Little(newCount);

                client->QueuePacket(p);

                updated = true;
            }
        }

        if(updated)
        {
            client->FlushOutgoing();

            server->GetWorldDatabase()->QueueUpdate(dQuest);

            return true;
        }
    }

    return false;
}

bool EventManager::ResetDemonQuests(const std::shared_ptr<
    ChannelClientConnection>& client)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto progress = character->GetProgress().Get();

    auto server = mServer.lock();
    auto characterManager = server->GetCharacterManager();

    std::list<std::shared_ptr<objects::Demon>> demons;
    for(auto& d : character->GetCOMP()->GetDemons())
    {
        if(!d.IsNull() && !d->GetHasQuest() &&
            characterManager->GetFamiliarityRank(d->GetFamiliarity()) >= 1)
        {
            demons.push_back(d.Get());
        }
    }

    if(demons.size() == 0 && progress->GetDemonQuestDaily() == 0)
    {
        return false;
    }

    auto dbChanges = libcomp::DatabaseChangeSet::Create();

    progress->SetDemonQuestDaily(0);
    dbChanges->Update(progress);

    // Notify the player if any demons have new quests
    libcomp::Packet request;
    if(demons.size() > 0)
    {
        request.WritePacketCode(
            ChannelToClientPacketCode_t::PACKET_DEMON_QUEST_LIST_UPDATED);

        request.WriteS8((int8_t)demons.size());
        for(auto& d : demons)
        {
            d->SetHasQuest(true);
            request.WriteS64Little(state->GetObjectID(d->GetUUID()));

            dbChanges->Update(d);
        }
    }

    if(!server->GetWorldDatabase()->ProcessChangeSet(dbChanges))
    {
        return false;
    }

    if(demons.size() > 0)
    {
        client->SendPacket(request);
    }

    return true;
}

int8_t EventManager::EndDemonQuest(
    const std::shared_ptr<ChannelClientConnection>& client, int8_t failCode)
{
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto progress = character->GetProgress().Get();
    auto dQuest = character->GetDemonQuest().Get();

    if(!dQuest || failCode < 0 || failCode > 3)
    {
        // No quest or invalid supplied failure code, nothing to do
        return -1;
    }

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();

    auto demon = std::dynamic_pointer_cast<objects::Demon>(
        libcomp::PersistentObject::GetObjectByUUID(dQuest->GetDemon()));
    if(!demon)
    {
        return -1;
    }

    auto dbChanges = libcomp::DatabaseChangeSet::Create(
        state->GetAccountUID());

    if(failCode)
    {
        // Fail/reject the quest
        character->SetDemonQuest(NULLUUID);
        demon->SetHasQuest(false);

        // If the quest was accepted, reset the sequential success count
        if(!dQuest->GetUUID().IsNull())
        {
            progress->SetDemonQuestSequence(0);
            dbChanges->Update(progress);
            dbChanges->Delete(dQuest);
        }

        dbChanges->Update(character);
        dbChanges->Update(demon);
    }
    else
    {
        auto effects = cState->GetStatusEffects();
        if(effects.find(SVR_CONST.STATUS_DEMON_QUEST_ACTIVE) == effects.end())
        {
            // Quest has expired
            return 1;
        }

        for(auto& targetPair : dQuest->GetTargets())
        {
            // Quest is not complete
            if(dQuest->GetTargetCurrentCounts(targetPair.first) <
                targetPair.second)
            {
                return -1;
            }
        }

        if(dQuest->GetType() == objects::DemonQuest::Type_t::ITEM)
        {
            // Remove the items now
            std::unordered_map<uint32_t, uint32_t> removeItems;
            for(auto& pair : dQuest->GetTargets())
            {
                removeItems[pair.first] = (uint32_t)pair.second;
            }

            if(!server->GetCharacterManager()->AddRemoveItems(client,
                removeItems, false))
            {
                return -1;
            }
        }

        // Complete the quest and remove it
        auto demonData = definitionManager->GetDevilData(demon->GetType());
        if(demonData)
        {
            uint8_t raceID = (uint8_t)demonData->GetCategory()->GetRace();
            uint32_t count = progress->GetDemonQuestsCompleted(raceID);

            progress->SetDemonQuestsCompleted(raceID, (uint16_t)(count + 1));
        }

        character->SetDemonQuest(NULLUUID);
        progress->SetDemonQuestSequence((uint16_t)(
            progress->GetDemonQuestSequence() + 1));
        demon->SetHasQuest(false);

        dbChanges->Update(character);
        dbChanges->Update(progress);
        dbChanges->Update(demon);
        dbChanges->Delete(dQuest);
    }

    UpdateQuestTargetEnemies(client);

    server->GetWorldDatabase()->ProcessChangeSet(dbChanges);

    // If the quest is active, notify the player
    if(!dQuest->GetUUID().IsNull() && failCode != 3)
    {
        libcomp::Packet notify;
        notify.WritePacketCode(
            ChannelToClientPacketCode_t::PACKET_DEMON_QUEST_END);
        notify.WriteS8(failCode);
        notify.WriteS16Little((int16_t)progress->GetDemonQuestSequence());
        notify.WriteS32Little(0);    // Unknown

        client->SendPacket(notify);
    }

    // Lastly remove the quest active status effect
    StatusEffectChanges effects;
    effects[SVR_CONST.STATUS_DEMON_QUEST_ACTIVE] = StatusEffectChange(
        SVR_CONST.STATUS_DEMON_QUEST_ACTIVE, 0, true);
    cState->AddStatusEffects(effects, definitionManager);

    return 0;
}

bool EventManager::HandleEvent(const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::EventInstance>& instance)
{
    if(client)
    {
        EventContext ctx;
        ctx.Client = client;
        ctx.EventInstance = instance;
        ctx.CurrentZone = client->GetClientState()->GetCharacterState()->GetZone();

        return HandleEvent(ctx);
    }
    else
    {
        return false;
    }
}

bool EventManager::HandleEvent(EventContext& ctx)
{
    if(ctx.EventInstance == nullptr)
    {
        // End the event sequence
        return EndEvent(ctx.Client);
    }
    else if(ctx.Client)
    {
        // If an event is already in progress that is not the one
        // requested, queue the requested event and stop
        auto state = ctx.Client->GetClientState();
        auto eState = state->GetEventState();
        if(eState->GetCurrent())
        {
            if(eState->GetCurrent() != ctx.EventInstance)
            {
                eState->AppendQueued(ctx.EventInstance);
                return true;
            }
        }
        else
        {
            eState->SetCurrent(ctx.EventInstance);
        }
    }

    ctx.EventInstance->SetState(ctx.EventInstance->GetEvent());

    bool handled = false;

    // If the event is conditional, check it now and end if it fails
    auto event = ctx.EventInstance->GetEvent();
    auto conditions = event->GetConditions();
    if(conditions.size() > 0 && !EvaluateEventConditions(ctx, conditions))
    {
        handled = true;
        EndEvent(ctx.Client);
    }
    else
    {
        auto server = mServer.lock();
        auto eventType = event->GetEventType();
        switch(eventType)
        {
            case objects::Event::EventType_t::NPC_MESSAGE:
                if(ctx.Client)
                {
                    server->GetCharacterManager()->SetStatusIcon(ctx.Client, 4);
                    handled = NPCMessage(ctx);
                }
                break;
            case objects::Event::EventType_t::EX_NPC_MESSAGE:
                if(ctx.Client)
                {
                    server->GetCharacterManager()->SetStatusIcon(ctx.Client, 4);
                    handled = ExNPCMessage(ctx);
                }
                break;
            case objects::Event::EventType_t::MULTITALK:
                if(ctx.Client)
                {
                    server->GetCharacterManager()->SetStatusIcon(ctx.Client, 4);
                    handled = Multitalk(ctx);
                }
                break;
            case objects::Event::EventType_t::PROMPT:
                if(ctx.Client)
                {
                    server->GetCharacterManager()->SetStatusIcon(ctx.Client, 4);
                    handled = Prompt(ctx);
                }
                break;
            case objects::Event::EventType_t::PLAY_SCENE:
                if(ctx.Client)
                {
                    server->GetCharacterManager()->SetStatusIcon(ctx.Client, 4);
                    handled = PlayScene(ctx);
                }
                break;
            case objects::Event::EventType_t::PERFORM_ACTIONS:
                handled = PerformActions(ctx);
                break;
            case objects::Event::EventType_t::OPEN_MENU:
                if(ctx.Client)
                {
                    server->GetCharacterManager()->SetStatusIcon(ctx.Client, 4);
                    handled = OpenMenu(ctx);
                }
                break;
            case objects::Event::EventType_t::DIRECTION:
                if(ctx.Client)
                {
                    server->GetCharacterManager()->SetStatusIcon(ctx.Client, 4);
                    handled = Direction(ctx);
                }
                break;
            case objects::Event::EventType_t::FORK:
                // Fork off to the next appropriate event but even if there are
                // no next events listed, allow the handler to take care of it
                HandleNext(ctx);
                handled = true;
                break;
            default:
                LOG_ERROR(libcomp::String("Failed to handle event of type %1\n"
                    ).Arg(to_underlying(eventType)));
                break;
        }

        if(!handled)
        {
            EndEvent(ctx.Client);
        }
    }

    return handled;
}

void EventManager::HandleNext(EventContext& ctx)
{
    auto state = ctx.Client ? ctx.Client->GetClientState() : nullptr;
    auto eState = state ? state->GetEventState() : nullptr;

    auto event = ctx.EventInstance->GetEvent();
    auto iState = ctx.EventInstance->GetState();
    libcomp::String nextEventID = iState->GetNext();
    libcomp::String queueEventID = iState->GetQueueNext();

    if(iState->BranchesCount() > 0)
    {
        libcomp::String branchScriptID = iState->GetBranchScriptID();
        if(!branchScriptID.IsEmpty())
        {
            // Branch based on an index result of a script representing
            // the branch number to use
            auto serverDataManager = mServer.lock()->GetServerDataManager();
            auto script = serverDataManager->GetScript(branchScriptID);
            if(script && script->Type.ToLower() == "eventbranchlogic")
            {
                auto engine = std::make_shared<libcomp::ScriptEngine>();
                engine->Using<CharacterState>();
                engine->Using<DemonState>();
                engine->Using<Zone>();
                engine->Using<libcomp::Randomizer>();

                if(engine->Eval(script->Source))
                {
                    Sqrat::Function f(Sqrat::RootTable(engine->GetVM()), "check");

                    Sqrat::Array sqParams(engine->GetVM());
                    for(libcomp::String p : iState->GetBranchScriptParams())
                    {
                        sqParams.Append(p);
                    }

                    auto scriptResult = !f.IsNull()
                        ? f.Evaluate<size_t>(
                            state ? state->GetCharacterState() : nullptr,
                            state ? state->GetDemonState() : nullptr,
                            ctx.CurrentZone,
                            sqParams) : 0;
                    if(scriptResult)
                    {
                        size_t idx = *scriptResult;
                        if(idx < iState->BranchesCount())
                        {
                            auto branch = iState->GetBranches(idx);
                            nextEventID = branch->GetNext();
                            queueEventID = branch->GetQueueNext();
                        }
                    }
                }
            }
            else
            {
                LOG_ERROR(libcomp::String("Invalid event branch script ID: %1\n")
                    .Arg(branchScriptID));
            }
        }
        else
        {
            // Branch based on conditions
            for(auto branch : iState->GetBranches())
            {
                auto conditions = branch->GetConditions();
                if(conditions.size() > 0 && EvaluateEventConditions(
                    ctx, conditions))
                {
                    // Use the branch instead (first to pass is used)
                    nextEventID = branch->GetNext();
                    queueEventID = branch->GetQueueNext();
                    break;
                }
            }
        }
    }

    if(!queueEventID.IsEmpty() && eState)
    {
        auto queue = PrepareEvent(queueEventID,
            ctx.EventInstance->GetSourceEntityID());
        if(queue)
        {
            eState->AppendQueued(queue);
        }
    }

    if(nextEventID.IsEmpty())
    {
        if(eState)
        {
            auto previous = eState->PreviousCount() > 0
                ? eState->GetPrevious().back() : nullptr;
            if(previous != nullptr && (iState->GetPop() || iState->GetPopNext()))
            {
                // Return to pop event
                eState->RemovePrevious(eState->PreviousCount() - 1);
                eState->SetCurrent(previous);

                ctx.EventInstance = previous;
                eState->SetCurrent(previous);

                HandleEvent(ctx);
                return;
            }
            else if(eState->QueuedCount() > 0)
            {
                // Process the first queued event
                auto queued = eState->GetQueued(0);
                eState->RemoveQueued(0);

                // Push current onto previous and replace
                eState->AppendPrevious(ctx.EventInstance);
                eState->SetCurrent(queued);

                HandleEvent(ctx.Client, queued);
                return;
            }
        }

        // End the sequence
        EndEvent(ctx.Client);
    }
    else
    {
        if(eState)
        {
            // Push current onto previous
            eState->AppendPrevious(ctx.EventInstance);
            eState->SetCurrent(nullptr);
        }

        HandleEvent(ctx.Client, nextEventID,
            ctx.EventInstance->GetSourceEntityID(), ctx.CurrentZone,
            ctx.EventInstance->GetActionGroupID());
    }
}

bool EventManager::NPCMessage(EventContext& ctx)
{
    auto e = std::dynamic_pointer_cast<objects::EventNPCMessage>(
        ctx.EventInstance->GetEvent());
    auto idx = ctx.EventInstance->GetIndex();
    auto unknown = e->GetUnknown((size_t)idx);

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_NPC_MESSAGE);
    p.WriteS32Little(ctx.EventInstance->GetSourceEntityID());
    p.WriteS32Little(e->GetMessageIDs((size_t)idx));
    p.WriteS32Little(unknown != 0 ? unknown : e->GetUnknownDefault());

    ctx.Client->SendPacket(p);

    return true;
}

bool EventManager::ExNPCMessage(EventContext& ctx)
{
    auto e = std::dynamic_pointer_cast<objects::EventExNPCMessage>(
        ctx.EventInstance->GetEvent());

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_EX_NPC_MESSAGE);
    p.WriteS32Little(ctx.EventInstance->GetSourceEntityID());
    p.WriteS32Little(e->GetMessageID());
    p.WriteS16Little(e->GetEx1());

    bool ex2Set = e->GetEx2() != 0;
    p.WriteS8(ex2Set ? 1 : 0);
    if(ex2Set)
    {
        p.WriteS32Little(e->GetEx2());
    }

    ctx.Client->SendPacket(p);

    return true;
}

bool EventManager::Multitalk(EventContext& ctx)
{
    auto e = std::dynamic_pointer_cast<objects::EventMultitalk>(
        ctx.EventInstance->GetEvent());

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_MULTITALK);
    p.WriteS32Little(e->GetPlayerSource()
        ? ctx.Client->GetClientState()->GetCharacterState()->GetEntityID()
        : ctx.EventInstance->GetSourceEntityID());
    p.WriteS32Little(e->GetMessageID());

    ctx.Client->SendPacket(p);

    return true;
}

bool EventManager::Prompt(EventContext& ctx)
{
    auto e = std::dynamic_pointer_cast<objects::EventPrompt>(
        ctx.EventInstance->GetEvent());

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_PROMPT);
    p.WriteS32Little(ctx.EventInstance->GetSourceEntityID() == 0
        ? ctx.Client->GetClientState()->GetCharacterState()->GetEntityID()
        : ctx.EventInstance->GetSourceEntityID());
    p.WriteS32Little(e->GetMessageID());

    ctx.EventInstance->ClearDisabledChoices();

    std::vector<std::shared_ptr<objects::EventChoice>> choices;
    for(size_t i = 0; i < e->ChoicesCount(); i++)
    {
        auto choice = e->GetChoices(i);

        auto conditions = choice->GetConditions();
        if(choice->GetMessageID() != 0 &&
            (conditions.size() == 0 || EvaluateEventConditions(ctx, conditions)))
        {
            choices.push_back(choice);
        }
        else
        {
            ctx.EventInstance->InsertDisabledChoices((uint8_t)i);
        }
    }

    size_t choiceCount = choices.size();
    p.WriteS32Little((int32_t)choiceCount);
    for(size_t i = 0; i < choiceCount; i++)
    {
        auto choice = choices[i];
        p.WriteS32Little((int32_t)i);
        p.WriteS32Little(choice->GetMessageID());
    }

    ctx.Client->SendPacket(p);

    return true;
}

bool EventManager::PlayScene(EventContext& ctx)
{
    auto e = std::dynamic_pointer_cast<objects::EventPlayScene>(
        ctx.EventInstance->GetEvent());

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_PLAY_SCENE);
    p.WriteS32Little(e->GetSceneID());
    p.WriteS8(e->GetUnknown());

    ctx.Client->SendPacket(p);

    return true;
}

bool EventManager::OpenMenu(EventContext& ctx)
{
    auto e = std::dynamic_pointer_cast<objects::EventOpenMenu>(
        ctx.EventInstance->GetEvent());
    auto state = ctx.Client->GetClientState();
    auto eState = state->GetEventState();

    int32_t menuType = e->GetMenuType();
    if(menuType == (int32_t)SVR_CONST.MENU_TRIFUSION &&
        !HandleTriFusion(ctx.Client))
    {
        return false;
    }

    int32_t overrideShopID = (int32_t)eState->GetCurrent()->GetShopID();

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_OPEN_MENU);
    p.WriteS32Little(ctx.EventInstance->GetSourceEntityID());
    p.WriteS32Little(menuType);
    p.WriteS32Little(overrideShopID != 0 ? overrideShopID : e->GetShopID());
    p.WriteString16Little(state->GetClientStringEncoding(),
        libcomp::String(), true);

    ctx.Client->SendPacket(p);

    return true;
}

bool EventManager::PerformActions(EventContext& ctx)
{
    auto e = std::dynamic_pointer_cast<objects::EventPerformActions>(
        ctx.EventInstance->GetEvent());

    auto server = mServer.lock();
    auto actionManager = server->GetActionManager();
    auto actions = e->GetActions();

    actionManager->PerformActions(ctx.Client, actions,
        ctx.EventInstance->GetSourceEntityID(), ctx.CurrentZone,
        ctx.EventInstance->GetActionGroupID());

    HandleNext(ctx);

    return true;
}

bool EventManager::Direction(EventContext& ctx)
{
    auto e = std::dynamic_pointer_cast<objects::EventDirection>(
        ctx.EventInstance->GetEvent());

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_DIRECTION);
    p.WriteS32Little(e->GetDirection());

    ctx.Client->SendPacket(p);

    return true;
}

bool EventManager::EndEvent(const std::shared_ptr<ChannelClientConnection>& client)
{
    if(client)
    {
        auto state = client->GetClientState();
        auto eState = state->GetEventState();

        eState->SetCurrent(nullptr);
        eState->ClearPrevious();
        eState->ClearQueued();

        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_END);

        client->SendPacket(p);

        auto server = mServer.lock();
        server->GetCharacterManager()->SetStatusIcon(client);
    }

    return true;
}

bool EventManager::HandleTriFusion(const std::shared_ptr<
    ChannelClientConnection>& client)
{
    auto state = client->GetClientState();

    if(state->GetExchangeSession())
    {
        // There is already an exchange session
        return false;
    }

    auto partyClients = mServer.lock()->GetManagerConnection()
        ->GetPartyConnections(client, true, true);

    ClientState* tfSessionOwner;
    std::shared_ptr<objects::TriFusionHostSession> tfSession;
    for(auto pClient : partyClients)
    {
        if(pClient == client) continue;

        auto pState = pClient->GetClientState();
        tfSession = std::dynamic_pointer_cast<
            objects::TriFusionHostSession>(pState->GetExchangeSession());
        if(tfSession)
        {
            tfSessionOwner = pState;
            break;
        }
    }

    if(tfSessionOwner)
    {
        // Request to prompt the client to join
        libcomp::Packet request;
        request.WritePacketCode(
            ChannelToClientPacketCode_t::PACKET_TRIFUSION_START);
        request.WriteS32Little(tfSessionOwner->GetCharacterState()->GetEntityID());

        client->QueuePacket(request);
    }
    else
    {
        // Send special notification to all party members in the zone 
        // (including self)
        tfSession = std::make_shared<objects::TriFusionHostSession>();
        tfSession->SetSourceEntityID(state->GetCharacterState()
            ->GetEntityID());

        state->SetExchangeSession(tfSession);

        libcomp::Packet notify;
        notify.WritePacketCode(
            ChannelToClientPacketCode_t::PACKET_TRIFUSION_STARTED);
        notify.WriteS32Little(state->GetCharacterState()->GetEntityID());

        ChannelClientConnection::BroadcastPacket(partyClients, notify);
    }

    return true;
}
