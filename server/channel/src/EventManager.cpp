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
 * Copyright (C) 2012-2017 COMP_hack Team <compomega@tutanota.com>
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
#include <Log.h>
#include <PacketCodes.h>
#include <Randomizer.h>
#include <ServerConstants.h>

// Standard C++11 Includes
#include <math.h>

// channel Includes
#include "ChannelServer.h"
#include "CharacterState.h"
#include "ZoneInstance.h"

// object Includes
#include <Account.h>
#include <CharacterProgress.h>
#include <DemonBox.h>
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
#include <MiDevilBookData.h>
#include <MiDevilData.h>
#include <MiExpertData.h>
#include <MiItemBasicData.h>
#include <MiItemData.h>
#include <MiQuestData.h>
#include <MiQuestPhaseData.h>
#include <MiQuestUpperCondition.h>
#include <MiUnionData.h>
#include <Party.h>
#include <Quest.h>
#include <QuestPhaseRequirement.h>
#include <ServerNPC.h>
#include <ServerObject.h>
#include <ServerZone.h>
#include <ServerZoneInstance.h>
#include <TriFusionHostSession.h>

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
    auto instance = PrepareEvent(client, eventID, sourceEntityID);
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
    const std::shared_ptr<ChannelClientConnection>& client,
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

        if(client)
        {
            auto state = client->GetClientState();
            auto eState = state->GetEventState();
            if(eState->GetCurrent() != nullptr)
            {
                eState->AppendPrevious(eState->GetCurrent());
            }

            eState->SetCurrent(instance);
        }

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
        LOG_ERROR(libcomp::String("Option selected for unknown event: %1\n"
            ).Arg(character->GetAccount()->GetUsername()));

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
    auto characterManager = server->GetCharacterManager();
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
    characterManager->ConvertIDToMaskValues((uint16_t)questID, index, shiftVal);

    uint8_t indexVal = progress->GetCompletedQuests(index);
    bool completed = (shiftVal & indexVal) != 0;

    auto dbChanges = libcomp::DatabaseChangeSet::Create(state->GetAccountUID());
    auto quest = character->GetQuests(questID).Get();
    bool sendUpdate = phase != -2;
    if(phase == -1)
    {
        // Completing a quest
        if(!quest && completed && !forceUpdate)
        {
            LOG_ERROR(libcomp::String("Quest '%1' has already been completed\n")
                .Arg(questID));
            return false;
        }

        progress->SetCompletedQuests(index, (uint8_t)(shiftVal | indexVal));
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
        quest->SetCharacter(character);
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
            server->GetCharacterManager()->ConvertIDToMaskValues((uint16_t)questID,
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
            server->GetCharacterManager()->ConvertIDToMaskValues((uint16_t)questID,
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
            int32_t lncType = (int32_t)client->GetClientState()->GetCharacterState()->GetLNCType();
            int32_t val1 = condition->GetValue1();
            if(val1 == 1)
            {
                // Not chaos
                return lncType == LNC_LAW || lncType == LNC_NEUTRAL;
            }
            else if(val1 == 3)
            {
                // Not law
                return lncType == LNC_NEUTRAL || lncType == LNC_CHAOS;;
            }
            else
            {
                // Explicitly law, neutral or chaos
                return lncType == val1;
            }
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
            auto items = mServer.lock()->GetCharacterManager()->GetExistingItems(character,
                (uint32_t)condition->GetValue1());

            uint16_t count = 0;
            for(auto item : items)
            {
                count = (uint16_t)(count + (uint16_t)item->GetStackSize());
            }

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

            return mServer.lock()->GetCharacterManager()->HasValuable(character, valuableID) != 
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
            mServer.lock()->GetCharacterManager()->ConvertIDToMaskValues(questID, index, shiftVal);

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
            mServer.lock()->GetCharacterManager()->ConvertIDToMaskValues(mapID, index, shiftVal);

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
            auto characterManager = server->GetCharacterManager();
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
                    characterManager->ConvertIDToMaskValues(
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
            case objects::EntityStateObject::EntityType_t::NPC:
                npcState = std::dynamic_pointer_cast<NPCState>(npc)
                    ->GetEntity()->GetState();
                break;
            case objects::EntityStateObject::EntityType_t::OBJECT:
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
            mServer.lock()->GetCharacterManager()->ConvertIDToMaskValues(pluginID, index, shiftVal);

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
                auto characterManager = server->GetCharacterManager();
                auto items = characterManager->GetExistingItems(character,
                    req->GetObjectID());

                uint16_t count = 0;
                for(auto item : items)
                {
                    count = (uint16_t)(count + item->GetStackSize());
                }

                if(count < (uint16_t)req->GetObjectCount())
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
                            state->GetCharacterState(),
                            state->GetDemonState(),
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
        eState->AppendQueued(queueEventID);
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
                HandleEvent(ctx);
                return;
            }
            else if(eState->QueuedCount() > 0)
            {
                // Process the first queued event
                queueEventID = eState->GetQueued(0);
                eState->RemoveQueued(0);

                // Queued events have no source associated to them
                HandleEvent(ctx.Client, queueEventID, 0, ctx.CurrentZone);
                return;
            }
        }

        // End the sequence
        EndEvent(ctx.Client);
    }
    else
    {
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

    if(tfSession)
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
