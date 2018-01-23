/**
 * @file server/channel/src/ActionManager.cpp
 * @ingroup channel
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Class to manage actions when triggering a spot or interacting with
 * an object/NPC.
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

#include "ActionManager.h"

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

// Standard C++11 Includes
#include <math.h>

// channel Includes
#include "ChannelServer.h"

// object Includes
#include <Action.h>
#include <ActionAddRemoveItems.h>
#include <ActionAddRemoveStatus.h>
#include <ActionCreateLoot.h>
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
#include <ActionUpdateQuest.h>
#include <ActionUpdateZoneFlags.h>
#include <ActionZoneChange.h>
#include <CharacterProgress.h>
#include <DemonBox.h>
#include <DropSet.h>
#include <MiSpotData.h>
#include <ObjectPosition.h>
#include <Party.h>
#include <Quest.h>
#include <ServerNPC.h>
#include <ServerObject.h>
#include <ServerZone.h>

using namespace channel;

struct channel::ActionContext
{
    std::shared_ptr<ChannelClientConnection> Client;
    std::shared_ptr<objects::Action> Action;
    int32_t SourceEntityID = 0;
    std::shared_ptr<Zone> CurrentZone;
};

ActionManager::ActionManager(const std::weak_ptr<ChannelServer>& server)
    : mServer(server)
{
    mActionHandlers[objects::Action::ActionType_t::ZONE_CHANGE] =
        &ActionManager::ZoneChange;
    mActionHandlers[objects::Action::ActionType_t::START_EVENT] =
        &ActionManager::StartEvent;
    mActionHandlers[objects::Action::ActionType_t::SET_HOMEPOINT] =
        &ActionManager::SetHomepoint;
    mActionHandlers[objects::Action::ActionType_t::SET_NPC_STATE] =
        &ActionManager::SetNPCState;
    mActionHandlers[objects::Action::ActionType_t::ADD_REMOVE_ITEMS] =
        &ActionManager::AddRemoveItems;
    mActionHandlers[objects::Action::ActionType_t::UPDATE_COMP] =
        &ActionManager::UpdateCOMP;
    mActionHandlers[objects::Action::ActionType_t::GRANT_XP] =
        &ActionManager::GrantXP;
    mActionHandlers[objects::Action::ActionType_t::DISPLAY_MESSAGE] =
        &ActionManager::DisplayMessage;
    mActionHandlers[objects::Action::ActionType_t::STAGE_EFFECT] =
        &ActionManager::StageEffect;
    mActionHandlers[objects::Action::ActionType_t::SPECIAL_DIRECTION] =
        &ActionManager::SpecialDirection;
    mActionHandlers[objects::Action::ActionType_t::PLAY_BGM] =
        &ActionManager::PlayBGM;
    mActionHandlers[objects::Action::ActionType_t::PLAY_SOUND_EFFECT] =
        &ActionManager::PlaySoundEffect;
    mActionHandlers[objects::Action::ActionType_t::UPDATE_FLAG] =
        &ActionManager::UpdateFlag;
    mActionHandlers[objects::Action::ActionType_t::UPDATE_LNC] =
        &ActionManager::UpdateLNC;
    mActionHandlers[objects::Action::ActionType_t::UPDATE_QUEST] =
        &ActionManager::UpdateQuest;
    mActionHandlers[objects::Action::ActionType_t::UPDATE_ZONE_FLAGS] =
        &ActionManager::UpdateZoneFlags;
    mActionHandlers[objects::Action::ActionType_t::SPAWN] =
        &ActionManager::Spawn;
    mActionHandlers[objects::Action::ActionType_t::CREATE_LOOT] =
        &ActionManager::CreateLoot;
}

ActionManager::~ActionManager()
{
}

void ActionManager::PerformActions(
    const std::shared_ptr<ChannelClientConnection>& client,
    const std::list<std::shared_ptr<objects::Action>>& actions,
    int32_t sourceEntityID, const std::shared_ptr<Zone>& zone)
{
    ActionContext ctx;
    ctx.Client = client;
    ctx.SourceEntityID = sourceEntityID;

    if(zone)
    {
        ctx.CurrentZone = zone;
    }
    else if(ctx.Client)
    {
        ctx.CurrentZone = mServer.lock()->GetZoneManager()
            ->GetZoneInstance(ctx.Client);
    }
    else
    {
        LOG_ERROR("Configurable actions cannot be performed"
            " without supplying a curent zone or source connection\n");
        return;
    }

    for(auto action : actions)
    {
        ctx.Action = action;

        auto it = mActionHandlers.find(action->GetActionType());

        if(mActionHandlers.end() == it)
        {
            LOG_ERROR(libcomp::String("Failed to parse action of type %1\n"
                ).Arg(to_underlying(action->GetActionType())));
        }
        else
        {
            auto srcCtx = action->GetSourceContext();
            if(srcCtx != objects::Action::SourceContext_t::SOURCE)
            {
                auto connectionManager = mServer.lock()->GetManagerConnection();

                // Execute once per source context character and quit if any fail
                bool failure = false;
                std::set<int32_t> worldCIDs;
                switch(srcCtx)
                {
                case objects::Action::SourceContext_t::PARTY:
                    {
                        auto sourceClient = client ? client : connectionManager
                            ->GetEntityClient(sourceEntityID, false);
                        if(!sourceClient)
                        {
                            failure = true;
                            break;
                        }

                        auto party = sourceClient->GetClientState()->GetParty();
                        if(party)
                        {
                            worldCIDs = party->GetMemberIDs();
                        }
                    }
                    break;
                case objects::Action::SourceContext_t::ZONE:
                    {
                        auto clients = ctx.CurrentZone->GetConnectionList();
                        for(auto c : clients)
                        {
                            worldCIDs.insert(c->GetClientState()->GetWorldCID());
                        }
                    }
                    break;
                default:
                    break;
                }

                if(!failure)
                {
                    for(auto worldCID : worldCIDs)
                    {
                        auto charClient = connectionManager->
                            GetEntityClient(worldCID, true);
                        auto cState = charClient ? charClient->GetClientState()
                            ->GetCharacterState() : nullptr;
                        if(cState && cState->GetZone() == ctx.CurrentZone)
                        {
                            ActionContext copyCtx = ctx;
                            copyCtx.Client = charClient;
                            copyCtx.SourceEntityID = cState->GetEntityID();

                            failure |= !it->second(*this, copyCtx);
                        }
                    }
                }

                if(failure)
                {
                    break;
                }
            }
            else if(!it->second(*this, ctx))
            {
                break;
            }
        }
    }
}

bool ActionManager::StartEvent(ActionContext& ctx)
{
    if(!ctx.Client)
    {
        LOG_ERROR("Attempted to start an event with no associated"
            " client connection\n");
        return false;
    }

    auto act = std::dynamic_pointer_cast<objects::ActionStartEvent>(ctx.Action);

    auto server = mServer.lock();
    auto eventManager = server->GetEventManager();

    eventManager->HandleEvent(ctx.Client, act->GetEventID(), ctx.SourceEntityID);
    
    return true;
}

bool ActionManager::ZoneChange(ActionContext& ctx)
{
    if(!ctx.Client)
    {
        LOG_ERROR("Attempted to execute a zone change action with no"
            " associated client connection\n");
        return false;
    }

    auto act = std::dynamic_pointer_cast<objects::ActionZoneChange>(ctx.Action);

    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();

    // Where is the character going?
    uint32_t zoneID = act->GetZoneID();
    uint32_t dynamicMapID = act->GetDynamicMapID();
    float x = act->GetDestinationX();
    float y = act->GetDestinationY();
    float rotation = act->GetDestinationRotation();

    uint32_t spotID = act->GetSpotID();
    if(spotID > 0)
    {
        // If a spot is specified, get a random point in that spot instead
        auto definitionManager = server->GetDefinitionManager();
        auto serverDataManager = server->GetServerDataManager();
        auto zoneDef = serverDataManager->GetZoneData(zoneID, dynamicMapID);

        auto spots = definitionManager->GetSpotData(zoneDef->GetDynamicMapID());
        auto spotIter = spots.find(spotID);
        if(spotIter != spots.end())
        {
            Point p = zoneManager->GetRandomSpotPoint(spotIter->second);
            x = p.x;
            y = p.y;
            rotation = spotIter->second->GetRotation();
        }
    }

    // Enter the new zone and always leave the old zone even if its the same.
    if(!zoneManager->EnterZone(ctx.Client, zoneID, dynamicMapID,
        x, y, rotation, true))
    {
        LOG_ERROR(libcomp::String("Failed to add client to zone"
            " %1. Closing the connection.\n").Arg(zoneID));

        ctx.Client->Close();

        return false;
    }

    // Update to point to the new zone
    ctx.CurrentZone = zoneManager->GetZoneInstance(ctx.Client);

    return true;
}

bool ActionManager::SetHomepoint(ActionContext& ctx)
{
    if(!ctx.Client)
    {
        LOG_ERROR("Attempted to execute a set homepoint action with no"
            " associated client connection\n");
        return false;
    }

    auto act = std::dynamic_pointer_cast<objects::ActionSetHomepoint>(ctx.Action);
    
    auto state = ctx.Client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();

    uint32_t zoneID = act->GetZoneID();
    uint32_t spotID = act->GetSpotID();

    auto zoneDef = mServer.lock()->GetServerDataManager()->GetZoneData(zoneID, 0);
    if(zoneID == 0 || !zoneDef)
    {
        LOG_ERROR("Attempted to execute a set homepoint action with an"
            " invalid zone ID specified\n");
        return false;
    }

    float xCoord = 0.f;
    float yCoord = 0.f;
    float rot = 0.f;
    if(!mServer.lock()->GetZoneManager()->GetSpotPosition(zoneDef->GetDynamicMapID(),
        spotID, xCoord, yCoord, rot))
    {
        LOG_ERROR("Attempted to execute a set homepoint action with an"
            " invalid spot ID specified\n");
        return false;
    }

    character->SetHomepointZone(zoneID);
    character->SetHomepointSpotID(spotID);

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_HOMEPOINT_UPDATE);
    p.WriteS32Little((int32_t)zoneID);
    p.WriteFloat(xCoord);
    p.WriteFloat(yCoord);

    ctx.Client->SendPacket(p);

    mServer.lock()->GetWorldDatabase()->QueueUpdate(character, state->GetAccountUID());

    return true;
}

bool ActionManager::AddRemoveItems(ActionContext& ctx)
{
    if(!ctx.Client)
    {
        LOG_ERROR("Attempted to add or remove items with no associated"
            " client connection\n");
        return false;
    }

    auto act = std::dynamic_pointer_cast<objects::ActionAddRemoveItems>(ctx.Action);

    auto server = mServer.lock();
    auto characterManager = server->GetCharacterManager();
    auto state = ctx.Client->GetClientState();
    auto cState = state->GetCharacterState();
    auto items = act->GetItems();

    std::unordered_map<uint32_t, uint32_t> adds;
    std::unordered_map<uint32_t, uint32_t> removes;
    for(auto itemPair : items)
    {
        if(itemPair.second < 0)
        {
            removes[itemPair.first] = (uint32_t)(-itemPair.second);
        }
        else if(itemPair.second > 0)
        {
            adds[itemPair.first] = (uint32_t)itemPair.second;
        }
    }

    if(!characterManager->AddRemoveItems(ctx.Client, adds, true) ||
        !characterManager->AddRemoveItems(ctx.Client, removes, false))
    {
        if(act->GetStopOnFailure())
        {
            if(!act->GetOnFailureEvent().IsEmpty())
            {
                server->GetEventManager()->HandleEvent(ctx.Client, act->GetOnFailureEvent(),
                    ctx.SourceEntityID);
            }

            return false;
        }

        return true;
    }

    if(adds.size() > 0 && act->GetNotify())
    {
        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_GET_ITEMS);
        p.WriteS8((int8_t)adds.size());
        for(auto entry : adds)
        {
            p.WriteU32Little(entry.first);              // Type
            p.WriteU16Little((uint16_t)entry.second);   // Quantity
        }

        ctx.Client->SendPacket(p);
    }

    return true;
}

bool ActionManager::AddRemoveStatus(ActionContext& ctx)
{
    if(!ctx.Client)
    {
        LOG_ERROR("Attempted to add or remove a status effect with"
            " no associated client connection\n");
        return false;
    }

    auto act = std::dynamic_pointer_cast<objects::ActionAddRemoveStatus>(ctx.Action);

    auto state = ctx.Client->GetClientState();
    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto tokuseiManager = server->GetTokuseiManager();

    AddStatusEffectMap statuses;
    for(auto pair : act->GetStatusStacks())
    {
        statuses[pair.first] =
            std::pair<uint8_t, bool>(pair.second, act->GetIsReplace());
    }

    if(statuses.size() > 0)
    {
        if(act->GetTargetType() == objects::ActionAddRemoveStatus::TargetType_t::CHARACTER ||
            act->GetTargetType() ==
            objects::ActionAddRemoveStatus::TargetType_t::CHARACTER_AND_PARTNER)
        {
            state->GetCharacterState()->AddStatusEffects(statuses, definitionManager);
        }

        if(act->GetTargetType() == objects::ActionAddRemoveStatus::TargetType_t::PARTNER ||
            act->GetTargetType() ==
            objects::ActionAddRemoveStatus::TargetType_t::CHARACTER_AND_PARTNER)
        {
            state->GetDemonState()->AddStatusEffects(statuses, definitionManager);
        }

        // Recalculate the character and demon
        tokuseiManager->Recalculate(state->GetCharacterState(), true);
    }

    return true;
}

bool ActionManager::UpdateCOMP(ActionContext& ctx)
{
    if(!ctx.Client)
    {
        LOG_ERROR("Attempted to update COMP with no associated"
            " client connection\n");
        return false;
    }

    auto act = std::dynamic_pointer_cast<objects::ActionUpdateCOMP>(ctx.Action);

    auto server = mServer.lock();
    auto characterManager = server->GetCharacterManager();
    auto state = ctx.Client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto dState = state->GetDemonState();
    auto progress = character->GetProgress().Get();
    auto comp = character->GetCOMP().Get();

    // First increase the COMP
    uint8_t maxSlots = progress->GetMaxCOMPSlots();
    if(act->GetMaxSlots() > 0 && act->GetMaxSlots() > progress->GetMaxCOMPSlots())
    {
        maxSlots = act->GetMaxSlots();
    }

    size_t freeCount = 0;
    for(uint8_t i = 0; i < maxSlots; i++)
    {
        auto slot = comp->GetDemons((size_t)i);
        if(slot.IsNull())
        {
            freeCount++;
        }
    }

    // Second remove demons to free up more slots
    std::unordered_map<uint32_t, std::list<std::shared_ptr<objects::Demon>>> remove;
    if(act->RemoveDemonsCount() > 0)
    {
        for(uint8_t i = 0; i < maxSlots; i++)
        {
            auto slot = comp->GetDemons((size_t)i);
            if(!slot.IsNull())
            {
                // If there are more than one specified, the ones near the
                // start of the COMP will be removed first
                uint32_t type = slot->GetType();
                if(act->RemoveDemonsKeyExists(type))
                {
                    if(act->GetRemoveDemons(type) == 0)
                    {
                        // Special case, must be summoned demon
                        if(dState->GetEntity() == slot.Get())
                        {
                            remove[type].push_back(slot.Get());
                        }
                    }
                    else if(act->GetRemoveDemons(type) > (uint8_t)remove[type].size())
                    {
                        remove[type].push_back(slot.Get());
                    }
                }
            }
        }

        for(auto pair : act->GetRemoveDemons())
        {
            if((pair.second == 0 && remove[pair.first].size() != 1) ||
                (pair.second != 0 && (uint8_t)remove[pair.first].size() < pair.second))
            {
                LOG_ERROR("One or more demons does not exist for COMP removal"
                    " request\n");
                return false;
            }
            else
            {
                freeCount = (uint8_t)(freeCount + pair.second);
            }
        }
    }

    // Last add demons
    std::unordered_map<std::shared_ptr<objects::MiDevilData>, uint8_t> add;
    if(act->AddDemonsCount() > 0)
    {
        auto definitionManager = server->GetDefinitionManager();
        for(auto pair : act->GetAddDemons())
        {
            auto demonData = definitionManager->GetDevilData(pair.first);
            if(demonData == nullptr)
            {
                LOG_ERROR(libcomp::String("Invalid demon ID encountered: %1\n")
                    .Arg(pair.first));
                return false;
            }

            if(freeCount < pair.second)
            {
                LOG_ERROR("Not enough slots free for COMP add request\n");
                return false;
            }

            freeCount = (uint8_t)(freeCount - pair.second);

            add[demonData] = pair.second;
        }
    }

    // Apply the changes
    if(maxSlots > progress->GetMaxCOMPSlots())
    {
        progress->SetMaxCOMPSlots(maxSlots);
        if(!progress->Update(server->GetWorldDatabase()))
        {
            LOG_ERROR("Failed to increase COMP size\n");
            return false;
        }
    }

    if(remove.size() > 0)
    {
        auto dbChanges = libcomp::DatabaseChangeSet::Create(state->GetAccountUID());
        dbChanges->Update(comp);

        std::set<int8_t> slots;
        for(auto pair : remove)
        {
            for(auto demon : pair.second)
            {
                int8_t slot = demon->GetBoxSlot();
                if(dState->GetEntity() == demon)
                {
                    characterManager->StoreDemon(ctx.Client);
                }

                slots.insert(slot);
                comp->SetDemons((size_t)slot, NULLUUID);
                characterManager->DeleteDemon(demon, dbChanges);
            }
        }

        characterManager->SendDemonBoxData(ctx.Client, comp->GetBoxID(), slots);

        server->GetWorldDatabase()->QueueChangeSet(dbChanges);
    }

    if(add.size() > 0)
    {
        for(auto pair : add)
        {
            for(uint8_t i = 0; i < pair.second; i++)
            {
                if(!characterManager->ContractDemon(ctx.Client, pair.first, 0))
                {
                    // Not really a good way to recover from this
                    LOG_ERROR("Failed to contract one or more demons for"
                        " COMP add request\n");
                    return false;
                }
            }
        }
    }

    return true;
}

bool ActionManager::GrantXP(ActionContext& ctx)
{
    if(!ctx.Client)
    {
        LOG_ERROR("Attempted to grant XP with no associated"
            " client connection\n");
        return false;
    }

    auto act = std::dynamic_pointer_cast<objects::ActionGrantXP>(ctx.Action);

    auto characterManager = mServer.lock()->GetCharacterManager();
    auto state = ctx.Client->GetClientState();

    std::list<std::shared_ptr<ActiveEntityState>> entityStates;
    if(act->GetTargetType() == objects::ActionGrantXP::TargetType_t::CHARACTER ||
        act->GetTargetType() == objects::ActionGrantXP::TargetType_t::CHARACTER_AND_PARTNER)
    {
        entityStates.push_back(state->GetCharacterState());
    }

    if(act->GetTargetType() == objects::ActionGrantXP::TargetType_t::PARTNER ||
        act->GetTargetType() == objects::ActionGrantXP::TargetType_t::CHARACTER_AND_PARTNER)
    {
        entityStates.push_back(state->GetDemonState());
    }

    for(auto eState : entityStates)
    {
        if(eState->Ready())
        {
            int64_t xp = act->GetXP();
            if(act->GetAdjustable())
            {
                xp = (int64_t)ceil((double)xp *
                    ((double)eState->GetCorrectValue(CorrectTbl::RATE_XP) * 0.01));
            }

            characterManager->ExperienceGain(ctx.Client, (uint64_t)xp,
                eState->GetEntityID());
        }
    }

    return true;
}

bool ActionManager::GrantSkills(ActionContext& ctx)
{
    if(!ctx.Client)
    {
        LOG_ERROR("Attempted to grant skills with no associated"
            " client connection\n");
        return false;
    }

    auto act = std::dynamic_pointer_cast<objects::ActionGrantSkills>(ctx.Action);

    auto characterManager = mServer.lock()->GetCharacterManager();
    auto state = ctx.Client->GetClientState();

    std::shared_ptr<ActiveEntityState> eState;
    switch(act->GetTargetType())
    {
    case objects::ActionGrantSkills::TargetType_t::CHARACTER:
        eState = state->GetCharacterState();
        if(act->GetSkillPoints() > 0)
        {
            characterManager->UpdateSkillPoints(ctx.Client,
                act->GetSkillPoints());
        }

        if(act->ExpertisePointsCount() > 0)
        {
            std::list<std::pair<uint8_t, int32_t>> expPoints;
            for(auto pair : act->GetExpertisePoints())
            {
                expPoints.push_back(std::pair<uint8_t, int32_t>(pair.first,
                    pair.second));
            }

            characterManager->UpdateExpertisePoints(ctx.Client, expPoints);
        }
        break;
    case objects::ActionGrantSkills::TargetType_t::PARTNER:
        eState = state->GetDemonState();
        if(act->GetSkillPoints() > 0)
        {
            LOG_ERROR("Attempted to grant skill points to a partner demon\n");
            return false;
        }

        if(act->ExpertisePointsCount() > 0)
        {
            LOG_ERROR("Attempted to grant expertise points to a partner demon\n");
            return false;
        }
        break;
    }

    if(eState->Ready())
    {
        for(uint32_t skillID : act->GetSkillIDs())
        {
            characterManager->LearnSkill(ctx.Client, eState->GetEntityID(),
                skillID);
        }
    }

    return true;
}

bool ActionManager::DisplayMessage(ActionContext& ctx)
{
    if(!ctx.Client)
    {
        LOG_ERROR("Attempted to execute a display message action with no"
            " associated client connection\n");
        return false;
    }

    auto act = std::dynamic_pointer_cast<objects::ActionDisplayMessage>(ctx.Action);

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_MESSAGE);

    for(auto msg : act->GetMessageIDs())
    {
        p.Seek(2);
        p.WriteS32Little(msg);

        ctx.Client->QueuePacketCopy(p);
    }

    ctx.Client->FlushOutgoing();

    return true;
}

bool ActionManager::StageEffect(ActionContext& ctx)
{
    if(!ctx.Client)
    {
        LOG_ERROR("Attempted to execute a stage effect action with no"
            " associated client connection\n");
        return false;
    }

    auto act = std::dynamic_pointer_cast<objects::ActionStageEffect>(ctx.Action);

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_STAGE_EFFECT);
    p.WriteS32Little(act->GetMessageID());
    p.WriteS8(act->GetEffect1());

    bool effect2Set = act->GetEffect2() != 0;
    p.WriteS8(effect2Set ? 1 : 0);
    if(effect2Set)
    {
        p.WriteS32Little(act->GetEffect2());
    }

    ctx.Client->QueuePacket(p);

    if(act->GetIncludeMessage())
    {
        p.Clear();
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_MESSAGE);
        p.WriteS32Little(act->GetMessageID());

        ctx.Client->QueuePacket(p);
    }

    ctx.Client->FlushOutgoing();

    return true;
}

bool ActionManager::SpecialDirection(ActionContext& ctx)
{
    if(!ctx.Client)
    {
        LOG_ERROR("Attempted to execute a special direction action with no"
            " associated client connection\n");
        return false;
    }

    auto act = std::dynamic_pointer_cast<objects::ActionSpecialDirection>(ctx.Action);

    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_SPECIAL_DIRECTION);
    p.WriteU8(act->GetSpecial1());
    p.WriteU8(act->GetSpecial2());
    p.WriteS32Little(act->GetDirection());

    ctx.Client->SendPacket(p);

    return true;
}

bool ActionManager::PlayBGM(ActionContext& ctx)
{
    if(!ctx.Client)
    {
        LOG_ERROR("Attempted to execute a play BGM action with no"
            " associated client connection\n");
        return false;
    }

    auto act = std::dynamic_pointer_cast<objects::ActionPlayBGM>(ctx.Action);

    libcomp::Packet p;

    if(act->GetIsStop())
    {
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_STOP_BGM);
        p.WriteS32Little(act->GetMusicID());
    }
    else
    {
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_PLAY_BGM);
        p.WriteS32Little(act->GetMusicID());
        p.WriteS32Little(act->GetFadeInDelay());
        p.WriteS32Little(act->GetUnknown());
    }

    ctx.Client->SendPacket(p);

    return true;
}

bool ActionManager::PlaySoundEffect(ActionContext& ctx)
{
    if(!ctx.Client)
    {
        LOG_ERROR("Attempted to execute a play sound effect action with no"
            " associated client connection\n");
        return false;
    }

    auto act = std::dynamic_pointer_cast<objects::ActionPlaySoundEffect>(ctx.Action);
    
    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_PLAY_SOUND_EFFECT);
    p.WriteS32Little(act->GetSoundID());
    p.WriteS32Little(act->GetDelay());

    ctx.Client->SendPacket(p);

    return true;
}

bool ActionManager::SetNPCState(ActionContext& ctx)
{
    auto act = std::dynamic_pointer_cast<objects::ActionSetNPCState>(ctx.Action);

    if(act->GetSourceClientOnly() && ctx.Client == nullptr)
    {
        LOG_ERROR("Source client NPC state change requested but no"
            " source client exists in the current context!\n");
        return false;
    }

    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();

    std::shared_ptr<objects::EntityStateObject> oNPCState;
    if(act->GetActorID() > 0)
    {
        oNPCState = ctx.CurrentZone->GetActor(act->GetActorID());
    }
    else
    {
        oNPCState = ctx.CurrentZone->GetServerObject(ctx.SourceEntityID);
    }

    if(!oNPCState)
    {
        LOG_ERROR("SetNPCState attempted on invalid target\n");
        return false;
    }

    std::shared_ptr<objects::ServerObject> oNPC;
    switch(oNPCState->GetEntityType())
    {
    case objects::EntityStateObject::EntityType_t::NPC:
        oNPC = std::dynamic_pointer_cast<NPCState>(oNPCState)->GetEntity();
        break;
    case objects::EntityStateObject::EntityType_t::OBJECT:
        oNPC = std::dynamic_pointer_cast<ServerObjectState>(oNPCState)->GetEntity();
        break;
    default:
        break;
    }

    if(oNPC && (act->GetSourceClientOnly() ||
        act->GetState() != oNPC->GetState()))
    {
        if(act->GetFrom() >= 0 && oNPC->GetState() != (uint8_t)act->GetFrom())
        {
            // Stop all actions past this point
            return false;
        }

        if(!act->GetSourceClientOnly())
        {
            oNPC->SetState(act->GetState());
        }

        auto npc = std::dynamic_pointer_cast<objects::ServerNPC>(oNPC);
        if(npc)
        {
            if(act->GetSourceClientOnly())
            {
                if(act->GetState() == 1)
                {
                    zoneManager->ShowEntity(ctx.Client, oNPCState->GetEntityID());
                }
                else
                {
                    std::list<std::shared_ptr<
                        ChannelClientConnection>> clients = { ctx.Client };
                    std::list<int32_t> entityIDs = { oNPCState->GetEntityID()  };
                    zoneManager->RemoveEntities(clients, entityIDs);
                }
            }
            else
            {
                if(act->GetState() == 1)
                {
                    zoneManager->ShowEntityToZone(ctx.CurrentZone, oNPCState->GetEntityID());
                }
                else
                {
                    std::list<int32_t> entityIDs = { oNPCState->GetEntityID() };
                    zoneManager->RemoveEntitiesFromZone(ctx.CurrentZone, entityIDs);
                }
            }
        }
        else
        {
            libcomp::Packet p;
            p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_NPC_STATE_CHANGE);
            p.WriteS32Little(oNPCState->GetEntityID());
            p.WriteU8(act->GetState());

            if(act->GetSourceClientOnly())
            {
                ctx.Client->SendPacket(p);
            }
            else
            {
                zoneManager->BroadcastPacket(ctx.CurrentZone, p);
            }
        }
    }

    return true;
}

bool ActionManager::UpdateFlag(ActionContext& ctx)
{
    if(!ctx.Client)
    {
        LOG_ERROR("Attempted to execute a player flag update action with no"
            " associated client connection\n");
        return false;
    }

    auto act = std::dynamic_pointer_cast<objects::ActionUpdateFlag>(ctx.Action);
    auto characterManager = mServer.lock()->GetCharacterManager();

    switch(act->GetFlagType())
    {
    case objects::ActionUpdateFlag::FlagType_t::MAP:
        characterManager->AddMap(ctx.Client, act->GetID());
        break;
    case objects::ActionUpdateFlag::FlagType_t::PLUGIN:
        characterManager->AddPlugin(ctx.Client, act->GetID());
        break;
    case objects::ActionUpdateFlag::FlagType_t::VALUABLE:
        characterManager->AddRemoveValuable(ctx.Client, act->GetID(),
            act->GetRemove());
        break;
    default:
        return false;
    }

    return true;
}

bool ActionManager::UpdateLNC(ActionContext& ctx)
{
    if(!ctx.Client)
    {
        LOG_ERROR("Attempted to execute a player LNC update action with no"
            " associated client connection\n");
        return false;
    }

    auto act = std::dynamic_pointer_cast<objects::ActionUpdateLNC>(ctx.Action);
    auto character = ctx.Client->GetClientState()->GetCharacterState()
        ->GetEntity();
    auto characterManager = mServer.lock()->GetCharacterManager();

    int16_t lnc = character->GetLNC();
    if(act->GetIsSet())
    {
        lnc = act->GetValue();
    }
    else
    {
        lnc = (int16_t)(lnc + act->GetValue());
    }

    characterManager->UpdateLNC(ctx.Client, lnc);

    return true;
}

bool ActionManager::UpdateQuest(ActionContext& ctx)
{
    if(!ctx.Client)
    {
        LOG_ERROR("Attempted to execute a quest update action with no"
            " associated client connection\n");
        return false;
    }

    auto act = std::dynamic_pointer_cast<objects::ActionUpdateQuest>(ctx.Action);
    auto server = mServer.lock();
    auto eventManager = server->GetEventManager();

    auto flagStates = act->GetFlagStates();
    if(flagStates.size() > 0 &&
        act->GetFlagSetMode() != objects::ActionUpdateQuest::FlagSetMode_t::UPDATE)
    {
        auto character = ctx.Client->GetClientState()->GetCharacterState()->GetEntity();
        auto quest = character->GetQuests(act->GetQuestID()).Get();
        auto existing = quest ? quest->GetFlagStates() : std::unordered_map<int32_t, int32_t>();

        switch(act->GetFlagSetMode())
        {
        case objects::ActionUpdateQuest::FlagSetMode_t::INCREMENT:
            for(auto pair : flagStates)
            {
                auto it = existing.find(pair.first);
                if(it != existing.end())
                {
                    pair.second = it->second + pair.second;
                }
            }
            break;
        case objects::ActionUpdateQuest::FlagSetMode_t::DECREMENT:
            for(auto pair : flagStates)
            {
                auto it = existing.find(pair.first);
                if(it != existing.end())
                {
                    pair.second = it->second - pair.second;
                }
                else
                {
                    pair.second = -pair.second;
                }
            }
            break;
        default:
            break;
        }
    }

    return eventManager->UpdateQuest(ctx.Client, act->GetQuestID(), act->GetPhase(), false,
        flagStates);
}

bool ActionManager::UpdateZoneFlags(ActionContext& ctx)
{
    auto act = std::dynamic_pointer_cast<objects::ActionUpdateZoneFlags>(ctx.Action);
    switch(act->GetSetMode())
    {
    case objects::ActionUpdateZoneFlags::SetMode_t::UPDATE:
        for(auto pair : act->GetFlagStates())
        {
            ctx.CurrentZone->SetFlagState(pair.first, pair.second);
        }
        break;
    case objects::ActionUpdateZoneFlags::SetMode_t::INCREMENT:
    case objects::ActionUpdateZoneFlags::SetMode_t::DECREMENT:
        {
            bool incr = act->GetSetMode() ==
                objects::ActionUpdateZoneFlags::SetMode_t::INCREMENT;

            int32_t val;
            for(auto pair : act->GetFlagStates())
            {
                if(!ctx.CurrentZone->GetFlagState(pair.first, val))
                {
                    val = 0;
                }

                val = val + (incr ? 1 : -1);
                ctx.CurrentZone->SetFlagState(pair.first, val);
            }
        }
        break;
    default:
        break;
    }

    return true;
}

bool ActionManager::Spawn(ActionContext& ctx)
{
    auto act = std::dynamic_pointer_cast<objects::ActionSpawn>(ctx.Action);
    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();

    return zoneManager->UpdateSpawnGroups(ctx.CurrentZone, true, 0, act);
}

bool ActionManager::CreateLoot(ActionContext& ctx)
{
    auto act = std::dynamic_pointer_cast<objects::ActionCreateLoot>(ctx.Action);

    auto server = mServer.lock();
    auto characterManager = server->GetCharacterManager();
    auto serverDataManager = server->GetServerDataManager();
    auto zoneManager = server->GetZoneManager();

    auto zone = ctx.CurrentZone;
    uint32_t dynamicMapID = zone->GetDefinition()->GetDynamicMapID();

    std::list<std::shared_ptr<objects::ObjectPosition>> locations;
    switch(act->GetPosition())
    {
    case objects::ActionCreateLoot::Position_t::ABS:
        locations = act->GetLocations();
        break;
    case objects::ActionCreateLoot::Position_t::SOURCE_RELATIVE:
        {
            auto source = zone->GetEntity(ctx.SourceEntityID);
            if(!source)
            {
                LOG_ERROR("Attempted to create source relative loot without"
                    " a valid source entity\n");
                return false;
            }

            auto loc = std::make_shared<objects::ObjectPosition>();
            loc->SetX(source->GetCurrentX());
            loc->SetY(source->GetCurrentY());
            loc->SetRotation(source->GetCurrentRotation());
            locations.push_back(loc);
        }
        break;
    default:
        break;
    }

    uint64_t lootTime = 0;
    if(act->GetExpirationTime() > 0.f)
    {
        uint64_t now = ChannelServer::GetServerTime();
        lootTime = (uint64_t)(now +
            (uint64_t)((double)act->GetExpirationTime() * 1000000.0));
    }

    auto zConnections = ctx.CurrentZone->GetConnectionList();
    auto firstClient = zConnections.size() > 0 ? zConnections.front()
        : nullptr;

    std::list<int32_t> entityIDs;
    for(auto loc : locations)
    {
        auto lBox = std::make_shared<objects::LootBox>();
        if(act->GetIsBossBox())
        {
            lBox->SetType(objects::LootBox::Type_t::BOSS_BOX);
        }
        else
        {
            lBox->SetType(objects::LootBox::Type_t::TREASURE_BOX);
        }
        lBox->SetLootTime(lootTime);

        auto drops = act->GetDrops();
        for(uint32_t dropSetID : act->GetDropSetIDs())
        {
            auto dropSet = serverDataManager->GetDropSetData(dropSetID);
            if(dropSet)
            {
                for(auto drop : dropSet->GetDrops())
                {
                    drops.push_back(drop);
                }
            }
        }

        characterManager->CreateLootFromDrops(lBox, drops, 0, true);

        auto lState = std::make_shared<LootBoxState>(lBox);

        float x = loc->GetX();
        float y = loc->GetY();
        float rot = loc->GetRotation();
        zoneManager->GetSpotPosition(dynamicMapID, loc->GetSpotID(),
            x, y, rot);

        lState->SetCurrentX(x);
        lState->SetCurrentY(y);
        lState->SetCurrentRotation(rot);

        lState->SetEntityID(server->GetNextEntityID());
        entityIDs.push_back(lState->GetEntityID());

        zone->AddLootBox(lState);

        if(firstClient)
        {
            zoneManager->SendLootBoxData(firstClient, lState, nullptr,
                true, true);
        }
    }
    
    if(lootTime != 0)
    {
        zoneManager->ScheduleEntityRemoval(lootTime, zone, entityIDs);
    }

    ChannelClientConnection::FlushAllOutgoing(zConnections);

    return true;
}
