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

// Standard C++11 Includes
#include <math.h>

// channel Includes
#include "ChannelServer.h"

// object Includes
#include <Account.h>
#include <CharacterProgress.h>
#include <EventChoice.h>
#include <EventCondition.h>
#include <EventConditionData.h>
#include <EventDirection.h>
#include <EventExNPCMessage.h>
#include <EventGetItem.h>
#include <EventHomepoint.h>
#include <EventInstance.h>
#include <EventMessage.h>
#include <EventMultitalk.h>
#include <EventNPCMessage.h>
#include <EventOpenMenu.h>
#include <EventPerformActions.h>
#include <EventPlayBGM.h>
#include <EventPlayScene.h>
#include <EventPlaySoundEffect.h>
#include <EventPrompt.h>
#include <EventSpecialDirection.h>
#include <EventStageEffect.h>
#include <EventStopBGM.h>
#include <EventState.h>
#include <Expertise.h>
#include <Item.h>
#include <MiExpertData.h>
#include <MiQuestData.h>
#include <MiQuestPhaseData.h>
#include <MiQuestUpperCondition.h>
#include <Quest.h>
#include <QuestPhaseRequirement.h>
#include <ServerZone.h>

using namespace channel;

EventManager::EventManager(const std::weak_ptr<ChannelServer>& server)
    : mServer(server)
{
}

EventManager::~EventManager()
{
}

bool EventManager::HandleEvent(
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
        return false;
    }
    else
    {
        auto state = client->GetClientState();
        auto eState = state->GetEventState();
        if(eState->GetCurrent() != nullptr)
        {
            eState->AppendPrevious(eState->GetCurrent());
        }

        auto instance = std::shared_ptr<objects::EventInstance>(
            new objects::EventInstance);
        instance->SetEvent(event);
        instance->SetSourceEntityID(sourceEntityID);

        eState->SetCurrent(instance);

        return HandleEvent(client, instance);
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
                auto e = std::dynamic_pointer_cast<objects::EventMessage>(event);

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
    
    HandleNext(client, current);

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
        if(!forceUpdate)
        {
            if(phase != 0)
            {
                LOG_ERROR(libcomp::String("Non-zero quest phase requested for update"
                    " on quest '%1' which has not been started\n").Arg(questID));
                return false;
            }
            else if(completed && questData->GetType() != 1)
            {
                LOG_ERROR(libcomp::String("Already completed non-repeatable quest '%1'"
                    " cannot be started again\n").Arg(questID));
                return false;
            }
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
        if(quest->GetPhase() >= phase)
        {
            // Nothing to do but not an error
            return true;
        }
        if(!forceUpdate && quest->GetPhase() != (int8_t)(phase - 1))
        {
            LOG_ERROR(libcomp::String("Invalid quest phase update requested"
                " for quest '%1': %2\n").Arg(questID).Arg(phase));
            return false;
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

bool EventManager::EvaluateQuestConditions(const std::shared_ptr<
    ChannelClientConnection>& client, int16_t questID)
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
            if(!EvaluateCondition(client, conditionSet->GetClauses((size_t)i)))
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

bool EventManager::EvaluateEventCondition(const std::shared_ptr<
    ChannelClientConnection>& client, const std::shared_ptr<
    objects::EventCondition>& condition)
{
    bool negate = condition->GetNegate();
    switch(condition->GetType())
    {
    case objects::EventCondition::Type_t::NORMAL:
        // Evaluate the event specific condition
        return negate != EvaluateCondition(client, condition->GetData());
    case objects::EventCondition::Type_t::ZONE_FLAGS:
        {
            auto zone = mServer.lock()->GetZoneManager()->GetZoneInstance(client);
            if(!zone)
            {
                return false;
            }

            std::unordered_map<int32_t, int32_t> flagStates;
            for(auto pair : condition->GetFlagStates())
            {
                int32_t val;
                if(zone->GetFlagState(pair.first, val))
                {
                    flagStates[pair.first] = val;
                }
            }

            return EvaluateFlagStates(flagStates, condition);
        }
        break;
    default:
        break;
    }

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    int16_t questID = condition->GetQuestID();
    auto quest = character->GetQuests(questID).Get();

    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto questData = definitionManager->GetQuestData((uint32_t)questID);

    switch(condition->GetType())
    {
    case objects::EventCondition::Type_t::MAX_QUESTS:
        return negate != (character->QuestsCount() >= QUEST_ACTIVE_MAX);
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

            return negate != (quest == nullptr && (!completed || questData->GetType() == 1)
                && EvaluateQuestConditions(client, condition->GetQuestID()));
        }
    case objects::EventCondition::Type_t::QUEST_PHASE:
        return negate != (quest &&
            ((condition->GetCompareMode() == objects::EventCondition::CompareMode_t::EQUAL &&
                quest->GetPhase() == condition->GetPhase()) ||
            (condition->GetCompareMode() == objects::EventCondition::CompareMode_t::GTE &&
                quest->GetPhase() >= condition->GetPhase()) ||
            (condition->GetCompareMode() == objects::EventCondition::CompareMode_t::LT &&
                quest->GetPhase() < condition->GetPhase())));
    case objects::EventCondition::Type_t::QUEST_PHASE_REQUIREMENTS:
        return negate != (quest &&
            EvaluateQuestPhaseRequirements(client, questID, condition->GetPhase()));
    case objects::EventCondition::Type_t::QUEST_FLAGS:
        if(!quest || (condition->GetPhase() > -1 &&
            quest->GetPhase() != condition->GetPhase()))
        {
            return negate;
        }
        else
        {
            auto flagStates = quest->GetFlagStates();
            return EvaluateFlagStates(flagStates, condition);
        }
        break;
    default:
        break;
    }

    // Always return false when invalid
    return false;
}

bool EventManager::EvaluateFlagStates(const std::unordered_map<int32_t,
    int32_t>& flagStates, const std::shared_ptr<objects::EventCondition>& condition)
{
    bool negate = condition->GetNegate();

    bool result = true;
    switch(condition->GetCompareMode())
    {
    case objects::EventCondition::CompareMode_t::EXISTS:
        for(auto pair : condition->GetFlagStates())
        {
            if(flagStates.find(pair.first) == flagStates.end())
            {
                result &= negate;
            }
        }
        break;
    case objects::EventCondition::CompareMode_t::LT_OR_NAN:
        // Flag specific less than or not a number (does not exist)
        for(auto pair : condition->GetFlagStates())
        {
            auto it = flagStates.find(pair.first);
            if(it != flagStates.end() && it->second >= pair.second)
            {
                result &= negate;
            }
        }
        break;
    case objects::EventCondition::CompareMode_t::LT:
        for(auto pair : condition->GetFlagStates())
        {
            auto it = flagStates.find(pair.first);
            if(it == flagStates.end() || it->second >= pair.second)
            {
                result &= negate;
            }
        }
        break;
    case objects::EventCondition::CompareMode_t::GTE:
        for(auto pair : condition->GetFlagStates())
        {
            auto it = flagStates.find(pair.first);
            if(it == flagStates.end() || it->second < pair.second)
            {
                result &= negate;
            }
        }
        break;
    case objects::EventCondition::CompareMode_t::EQUAL:
    default:
        for(auto pair : condition->GetFlagStates())
        {
            auto it = flagStates.find(pair.first);
            if(it == flagStates.end() || it->second != pair.second)
            {
                result &= negate;
            }
        }
        break;
    }

    return result;
}

bool EventManager::EvaluateEventConditions(const std::shared_ptr<
    ChannelClientConnection>& client, const std::list<std::shared_ptr<
    objects::EventCondition>>& conditions)
{
    for(auto condition : conditions)
    {
        if(!EvaluateEventCondition(client, condition))
        {
            return false;
        }
    }

    return true;
}

bool EventManager::EvaluateCondition(const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::EventConditionData>& condition)
{
    auto server = mServer.lock();
    auto characterManager = server->GetCharacterManager();
    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();

    switch(condition->GetType())
    {
    case objects::EventConditionData::Type_t::LEVEL:
        {
            // Character level >= [value 1]
            auto character = cState->GetEntity();
            auto stats = character->GetCoreStats();
            return stats->GetLevel() >= condition->GetValue1();
        }
    case objects::EventConditionData::Type_t::LNC:
        {
            // Character LNC matches [value 1]
            int32_t lncType = (int32_t)cState->GetLNCType();
            int32_t val1 = condition->GetValue1();
            if(val1 == 1)
            {
                // Not chaos
                return lncType == LNC_LAW || lncType == LNC_NEUTRAL;
            }
            else if(val1 == 3)
            {
                // Not law
                return lncType == LNC_NEUTRAL || lncType == LNC_CHAOS;
            }
            else
            {
                // Explicitly law, neutral or chaos
                return lncType == val1;
            }
        }
    case objects::EventConditionData::Type_t::ITEM:
        {
            // Item of type = [value 1] with quantity of at least
            // [value 2] in the character's inventory
            auto character = cState->GetEntity();
            auto items = characterManager->GetExistingItems(character,
                (uint32_t)condition->GetValue1());

            uint16_t count = 0;
            for(auto item : items)
            {
                count = (uint16_t)(count + item->GetStackSize());
            }

            return count >= (uint16_t)condition->GetValue2();
        }
    case objects::EventConditionData::Type_t::VALUABLE:
        {
            // Valuable flag [value 1] = [value 2]
            auto character = cState->GetEntity();
            auto progress = character->GetProgress().Get();

            uint16_t valuableID = (uint16_t)condition->GetValue1();

            size_t index;
            uint8_t shiftVal;
            characterManager->ConvertIDToMaskValues(valuableID, index, shiftVal);

            uint8_t indexVal = progress->GetValuables(index);

            return ((indexVal & shiftVal) == 0) == (condition->GetValue2() == 0);
        }
    case objects::EventConditionData::Type_t::QUEST_COMPLETE:
        {
            // Complete quest flag [value 1] = [value 2]
            auto character = cState->GetEntity();
            auto progress = character->GetProgress().Get();

            uint16_t questID = (uint16_t)condition->GetValue1();

            size_t index;
            uint8_t shiftVal;
            characterManager->ConvertIDToMaskValues(questID, index, shiftVal);

            uint8_t indexVal = progress->GetCompletedQuests(index);

            return ((indexVal & shiftVal) == 0) == (condition->GetValue2() == 0);
        }
    case objects::EventConditionData::Type_t::TIMESPAN:
        {
            // Server time between [value 1] and [value 2] (format: HHmm)
            int8_t phase, hour, min;
            server->GetWorldClockTime(phase, hour, min);

            int8_t minHours = (int8_t)floorl((float)condition->GetValue1() * 0.01);
            int8_t minMinutes = (int8_t)(condition->GetValue1() - (minHours * 100));

            int8_t maxHours = (int8_t)floorl((float)condition->GetValue2() * 0.01);
            int8_t maxMinutes = (int8_t)(condition->GetValue2() - (maxHours * 100));

            uint16_t serverSum = (uint16_t)((hour * 60) + min);
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
    case objects::EventConditionData::Type_t::TIMESPAN_REAL:
        {
            // System time between [value 1] and [value 2] (format: ddHHmm)
            // Days are represented as Sunday = 0, Monday = 1, etc
            time_t systemTime(0);
            tm* t = gmtime(&systemTime);
            auto systemDay = t->tm_wday;
            auto systemHour = t->tm_hour;
            auto systemMinutes = t->tm_min;

            int8_t minDays = (int8_t)floorl((float)condition->GetValue1() * 0.0001);
            int8_t minHours = (int8_t)floorl((float)(condition->GetValue1() -
                (minDays * 10000)) * 0.01);
            int8_t minMinutes = (int8_t)floorl((float)(condition->GetValue1() -
                (minDays * 10000) - (minHours * 100)) * 0.01);

            int8_t maxDays = (int8_t)floorl((float)condition->GetValue2() * 0.0001);
            int8_t maxHours = (int8_t)floorl((float)(condition->GetValue2() -
                (maxDays * 10000)) * 0.01);
            int8_t maxMinutes = (int8_t)floorl((float)(condition->GetValue2() -
                (maxDays * 10000) - (maxHours * 100)) * 0.01);

            uint16_t systemSum = (uint16_t)((systemDay * 24 * 60 * 60) +
                (systemHour * 60) + systemMinutes);
            uint16_t minSum = (uint16_t)((minDays * 24 * 60 * 60) +
                (minHours * 60) + minMinutes);
            uint16_t maxSum = (uint16_t)((maxDays * 24 * 60 * 60) +
                (maxHours * 60) + maxMinutes);

            return minSum <= systemSum && systemSum <= maxSum;
        }
    case objects::EventConditionData::Type_t::MOON_PHASE:
        {
            // Server moon phase = [value 1]
            int8_t phase, hour, min;
            server->GetWorldClockTime(phase, hour, min);

            return phase == condition->GetValue1();
        }
    case objects::EventConditionData::Type_t::MAP:
        {
            // Map flag [value 1] = [value 2]
            auto character = cState->GetEntity();
            auto progress = character->GetProgress().Get();

            uint16_t mapID = (uint16_t)condition->GetValue1();

            size_t index;
            uint8_t shiftVal;
            characterManager->ConvertIDToMaskValues(mapID, index, shiftVal);

            uint8_t indexVal = progress->GetMaps(index);

            return ((indexVal & shiftVal) == 0) == (condition->GetValue2() == 0);
        }
    case objects::EventConditionData::Type_t::QUEST_ACTIVE:
        {
            // Quest ID [value 1] active check = [value 2] (1 for active, 0 for not active)
            auto character = cState->GetEntity();

            return character->GetQuests(
                (int16_t)condition->GetValue1()).IsNull() == (condition->GetValue2() == 0);
        }
    case objects::EventConditionData::Type_t::QUEST_SEQUENCE:
        {
            // Quest ID [value 1] is on its final phase (since this will progress the story)
            int16_t prevQuestID = (int16_t)condition->GetValue1();
            auto character = cState->GetEntity();
            auto prevQuest = character->GetQuests(prevQuestID).Get();

            if(prevQuest == nullptr)
            {
                return false;
            }

            auto definitionManager = server->GetDefinitionManager();
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
        {
            // Expertise ID [value 1] is not maxed out
            auto definitionManager = server->GetDefinitionManager();
            auto expDef = definitionManager->GetExpertClassData((uint32_t)condition->GetValue1());
            if(expDef == nullptr)
            {
                LOG_ERROR(libcomp::String("Invalid expertise ID supplied for"
                    " EvaluateCondition: %1\n").Arg(condition->GetValue1()));
                return false;
            }

            auto character = cState->GetEntity();
            auto exp = character->GetExpertises((size_t)condition->GetValue1()).Get();
            int32_t maxPoints = (expDef->GetMaxClass() * 100 * 1000)
                + (expDef->GetMaxRank() * 100 * 100);

            return exp == nullptr || (exp->GetPoints() < maxPoints);
        }
    case objects::EventConditionData::Type_t::EXPERTISE_ABOVE:
        {
            // Expertise ID [value 1] points >= [value 2] (points or class check)
            auto character = cState->GetEntity();
            auto exp = character->GetExpertises((size_t)condition->GetValue1()).Get();

            if(exp == nullptr)
            {
                // 0 points allocated to expertise
                return false;
            }

            int32_t val = condition->GetValue2();
            if(val <= 10)
            {
                // Class check
                int32_t currentClass = (int32_t)floorl((float)exp->GetPoints() * 0.00001f);
                return currentClass >= val;
            }
            else
            {
                // Check points
                return exp->GetPoints() >= val;
            }
        }
    case objects::EventConditionData::Type_t::SI_EQUIPPED:
        {
            LOG_ERROR("Currently unsupported soul infusion equipment check encountered"
                " in EvaluateCondition: %1\n");
            return false;
        }
    case objects::EventConditionData::Type_t::SUMMONED:
        {
            // Partner demon of type [value 1] is currently summoned
            auto dState = state->GetDemonState();
            auto demon = dState->GetEntity();

            return demon != nullptr &&
                demon->GetType() == (uint32_t)condition->GetValue1();
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
    instance->SetState(instance->GetEvent());

    bool handled = false;

    // If the event is conditional, check it now and end if it fails
    auto event = instance->GetEvent();
    auto conditions = event->GetConditions();
    if(conditions.size() > 0 && !EvaluateEventConditions(client, conditions))
    {
        handled = true;
        EndEvent(client);
    }
    else
    {
        auto server = mServer.lock();
        auto eventType = event->GetEventType();
        switch(eventType)
        {
            case objects::Event::EventType_t::MESSAGE:
                server->GetCharacterManager()->SetStatusIcon(client, 4);
                handled = Message(client, instance);
                break;
            case objects::Event::EventType_t::NPC_MESSAGE:
                server->GetCharacterManager()->SetStatusIcon(client, 4);
                handled = NPCMessage(client, instance);
                break;
            case objects::Event::EventType_t::EX_NPC_MESSAGE:
                server->GetCharacterManager()->SetStatusIcon(client, 4);
                handled = ExNPCMessage(client, instance);
                break;
            case objects::Event::EventType_t::MULTITALK:
                server->GetCharacterManager()->SetStatusIcon(client, 4);
                handled = Multitalk(client, instance);
                break;
            case objects::Event::EventType_t::PROMPT:
                server->GetCharacterManager()->SetStatusIcon(client, 4);
                handled = Prompt(client, instance);
                break;
            case objects::Event::EventType_t::PLAY_SCENE:
                server->GetCharacterManager()->SetStatusIcon(client, 4);
                handled = PlayScene(client, instance);
                break;
            case objects::Event::EventType_t::PERFORM_ACTIONS:
                handled = PerformActions(client, instance);
                break;
            case objects::Event::EventType_t::OPEN_MENU:
                server->GetCharacterManager()->SetStatusIcon(client, 4);
                handled = OpenMenu(client, instance);
                break;
            case objects::Event::EventType_t::GET_ITEMS:
                handled = GetItems(client, instance);
                break;
            case objects::Event::EventType_t::HOMEPOINT:
                handled = Homepoint(client, instance);
                break;
            case objects::Event::EventType_t::STAGE_EFFECT:
                handled = StageEffect(client, instance);
                break;
            case objects::Event::EventType_t::DIRECTION:
                server->GetCharacterManager()->SetStatusIcon(client, 4);
                handled = Direction(client, instance);
                break;
            case objects::Event::EventType_t::SPECIAL_DIRECTION:
                handled = SpecialDirection(client, instance);
                break;
            case objects::Event::EventType_t::PLAY_SOUND_EFFECT:
                handled = PlaySoundEffect(client, instance);
                break;
            case objects::Event::EventType_t::PLAY_BGM:
                handled = PlayBGM(client, instance);
                break;
            case objects::Event::EventType_t::STOP_BGM:
                handled = StopBGM(client, instance);
                break;
            case objects::Event::EventType_t::FORK:
                // Fork off to the next appropriate event but even if there are
                // no next events listed, allow the handler to take care of it
                HandleNext(client, instance);
                handled = true;
                break;
            default:
                LOG_ERROR(libcomp::String("Failed to handle event of type %1\n"
                    ).Arg(to_underlying(eventType)));
                break;
        }

        if(!handled)
        {
            EndEvent(client);
        }
    }

    return handled;
}

void EventManager::HandleNext(const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::EventInstance>& current)
{
    auto state = client->GetClientState();
    auto eState = state->GetEventState();
    auto event = current->GetEvent();
    auto iState = current->GetState();
    auto nextEventID = iState->GetNext();

    for(auto branch : iState->GetBranches())
    {
        auto conditions = branch->GetConditions();
        if(conditions.size() > 0 && EvaluateEventConditions(client, conditions))
        {
            // Use the branch instead (first to pass is used)
            nextEventID = branch->GetNext();
            break;
        }
    }

    if(nextEventID.IsEmpty())
    {
        auto previous = eState->PreviousCount() > 0 ? eState->GetPrevious().back() : nullptr;
        if(previous != nullptr && (iState->GetPop() || iState->GetPopNext()))
        {
            eState->RemovePrevious(eState->PreviousCount() - 1);
            eState->SetCurrent(previous);
            HandleEvent(client, previous);
        }
        else
        {
            EndEvent(client);
        }
    }
    else
    {
        HandleEvent(client, nextEventID, current->GetSourceEntityID());
    }
}

bool EventManager::Message(const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::EventInstance>& instance)
{
    auto e = std::dynamic_pointer_cast<objects::EventMessage>(instance->GetEvent());

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_MESSAGE);

    for(auto msg : e->GetMessageIDs())
    {
        p.Seek(2);
        p.WriteS32Little(msg);

        client->QueuePacketCopy(p);
    }

    client->FlushOutgoing();

    HandleNext(client, instance);

    return true;
}

bool EventManager::NPCMessage(const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::EventInstance>& instance)
{
    auto e = std::dynamic_pointer_cast<objects::EventNPCMessage>(instance->GetEvent());
    auto idx = instance->GetIndex();
    auto unknown = e->GetUnknown((size_t)idx);

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_NPC_MESSAGE);
    p.WriteS32Little(instance->GetSourceEntityID());
    p.WriteS32Little(e->GetMessageIDs((size_t)idx));
    p.WriteS32Little(unknown != 0 ? unknown : e->GetUnknownDefault());

    client->SendPacket(p);

    return true;
}

bool EventManager::ExNPCMessage(const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::EventInstance>& instance)
{
    auto e = std::dynamic_pointer_cast<objects::EventExNPCMessage>(instance->GetEvent());

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_EX_NPC_MESSAGE);
    p.WriteS32Little(instance->GetSourceEntityID());
    p.WriteS32Little(e->GetMessageID());
    p.WriteS16Little(e->GetEx1());

    bool ex2Set = e->GetEx2() != 0;
    p.WriteS8(ex2Set ? 1 : 0);
    if(ex2Set)
    {
        p.WriteS32Little(e->GetEx2());
    }

    client->SendPacket(p);

    return true;
}

bool EventManager::Multitalk(const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::EventInstance>& instance)
{
    auto e = std::dynamic_pointer_cast<objects::EventMultitalk>(instance->GetEvent());

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_MULTITALK);
    p.WriteS32Little(instance->GetSourceEntityID());
    p.WriteS32Little(e->GetMessageID());

    client->SendPacket(p);

    return true;
}

bool EventManager::Prompt(const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::EventInstance>& instance)
{
    auto e = std::dynamic_pointer_cast<objects::EventPrompt>(instance->GetEvent());

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_PROMPT);
    p.WriteS32Little(instance->GetSourceEntityID());
    p.WriteS32Little(e->GetMessageID());

    instance->ClearDisabledChoices();

    std::vector<std::shared_ptr<objects::EventChoice>> choices;
    for(size_t i = 0; i < e->ChoicesCount(); i++)
    {
        auto choice = e->GetChoices(i);

        auto conditions = choice->GetConditions();
        if(choice->GetMessageID() != 0 &&
            (conditions.size() == 0 || EvaluateEventConditions(client, conditions)))
        {
            choices.push_back(choice);
        }
        else
        {
            instance->InsertDisabledChoices((uint8_t)i);
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

    client->SendPacket(p);

    return true;
}

bool EventManager::PlayScene(const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::EventInstance>& instance)
{
    auto e = std::dynamic_pointer_cast<objects::EventPlayScene>(instance->GetEvent());

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_PLAY_SCENE);
    p.WriteS32Little(e->GetSceneID());
    p.WriteS8(e->GetUnknown());

    client->SendPacket(p);

    return true;
}

bool EventManager::OpenMenu(const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::EventInstance>& instance)
{
    auto e = std::dynamic_pointer_cast<objects::EventOpenMenu>(instance->GetEvent());
    auto state = client->GetClientState();

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_OPEN_MENU);
    p.WriteS32Little(instance->GetSourceEntityID());
    p.WriteS32Little(e->GetMenuType());
    p.WriteS32Little(e->GetShopID());
    p.WriteString16Little(state->GetClientStringEncoding(),
        libcomp::String(), true);

    client->SendPacket(p);

    return true;
}

bool EventManager::GetItems(const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::EventInstance>& instance)
{
    auto e = std::dynamic_pointer_cast<objects::EventGetItem>(instance->GetEvent());
    auto server = mServer.lock();

    // Cast to the correct stack size when adding
    std::unordered_map<uint32_t, uint32_t> items;
    for(auto entry : e->GetItems())
    {
        items[entry.first] = (uint32_t)entry.second;
    }

    bool doNext = true;
    if(server->GetCharacterManager()->AddRemoveItems(client, items, true))
    {
        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_GET_ITEMS);
        p.WriteS8((int8_t)items.size());
        for(auto entry : e->GetItems())
        {
            p.WriteU32Little(entry.first);      // Item type
            p.WriteU16Little(entry.second);     // Item quantity
        }

        client->SendPacket(p);
    }
    else if(!e->GetOnFailure().IsEmpty())
    {
        if(!HandleEvent(client, e->GetOnFailure(), instance->GetSourceEntityID()))
        {
            EndEvent(client);
        }
        doNext = false;
    }

    if(doNext)
    {
        HandleNext(client, instance);
    }

    return true;
}

bool EventManager::PerformActions(const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::EventInstance>& instance)
{
    auto e = std::dynamic_pointer_cast<objects::EventPerformActions>(instance->GetEvent());

    auto server = mServer.lock();
    auto actionManager = server->GetActionManager();
    auto actions = e->GetActions();

    actionManager->PerformActions(client, actions, instance->GetSourceEntityID());

    HandleNext(client, instance);

    return true;
}

bool EventManager::Homepoint(const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::EventInstance>& instance)
{
    auto e = std::dynamic_pointer_cast<objects::EventHomepoint>(instance->GetEvent());

    auto state = client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    uint32_t zoneID = 0;
    float xCoord = 0.f;
    float yCoord = 0.f;
    if(e->GetZoneID() > 0)
    {
        zoneID = e->GetZoneID();
        xCoord = e->GetX();
        yCoord = e->GetY();
    }
    else
    {
        // Use current position
        auto zone = cState->GetZone();
        zoneID = zone->GetDefinition()->GetID();
        xCoord = cState->GetCurrentX();
        yCoord = cState->GetCurrentY();
    }

    character->SetHomepointZone(zoneID);
    character->SetHomepointX(xCoord);
    character->SetHomepointY(yCoord);

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_HOMEPOINT_UPDATE);
    p.WriteS32Little((int32_t)zoneID);
    p.WriteFloat(xCoord);
    p.WriteFloat(yCoord);

    client->SendPacket(p);

    mServer.lock()->GetWorldDatabase()->QueueUpdate(character, state->GetAccountUID());

    HandleNext(client, instance);

    return true;
}

bool EventManager::StageEffect(const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::EventInstance>& instance)
{
    auto e = std::dynamic_pointer_cast<objects::EventStageEffect>(instance->GetEvent());

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_STAGE_EFFECT);
    p.WriteS32Little(e->GetMessageID());
    p.WriteS8(e->GetEffect1());

    bool effect2Set = e->GetEffect2() != 0;
    p.WriteS8(effect2Set ? 1 : 0);
    if(effect2Set)
    {
        p.WriteS32Little(e->GetEffect2());
    }

    client->SendPacket(p);

    HandleNext(client, instance);

    return true;
}

bool EventManager::Direction(const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::EventInstance>& instance)
{
    auto e = std::dynamic_pointer_cast<objects::EventDirection>(instance->GetEvent());

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_DIRECTION);
    p.WriteS32Little(e->GetDirection());

    client->SendPacket(p);

    return true;
}

bool EventManager::SpecialDirection(const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::EventInstance>& instance)
{
    auto e = std::dynamic_pointer_cast<objects::EventSpecialDirection>(instance->GetEvent());

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_SPECIAL_DIRECTION);
    p.WriteU8(e->GetSpecial1());
    p.WriteU8(e->GetSpecial2());
    p.WriteS32Little(e->GetDirection());

    client->SendPacket(p);

    HandleNext(client, instance);

    return true;
}

bool EventManager::PlaySoundEffect(const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::EventInstance>& instance)
{
    auto e = std::dynamic_pointer_cast<objects::EventPlaySoundEffect>(instance->GetEvent());

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_PLAY_SOUND_EFFECT);
    p.WriteS32Little(e->GetSoundID());
    p.WriteS32Little(e->GetDelay());

    client->SendPacket(p);

    HandleNext(client, instance);

    return true;
}

bool EventManager::PlayBGM(const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::EventInstance>& instance)
{
    auto e = std::dynamic_pointer_cast<objects::EventPlayBGM>(instance->GetEvent());

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_PLAY_BGM);
    p.WriteS32Little(e->GetMusicID());
    p.WriteS32Little(e->GetFadeInDelay());
    p.WriteS32Little(e->GetUnknown());

    client->SendPacket(p);

    HandleNext(client, instance);

    return true;
}


bool EventManager::StopBGM(const std::shared_ptr<ChannelClientConnection>& client,
    const std::shared_ptr<objects::EventInstance>& instance)
{
    auto e = std::dynamic_pointer_cast<objects::EventStopBGM>(instance->GetEvent());

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_STOP_BGM);
    p.WriteS32Little(e->GetMusicID());

    client->SendPacket(p);

    HandleNext(client, instance);

    return true;
}

bool EventManager::EndEvent(const std::shared_ptr<ChannelClientConnection>& client)
{
    auto state = client->GetClientState();
    auto eState = state->GetEventState();

    eState->SetCurrent(nullptr);

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_END);

    client->SendPacket(p);

    auto server = mServer.lock();
    server->GetCharacterManager()->SetStatusIcon(client);

    return true;
}
