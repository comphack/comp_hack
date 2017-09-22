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

// channel Includes
#include "ChannelServer.h"

// object Includes
#include <Action.h>
#include <ActionAddRemoveItems.h>
#include <ActionCreateLoot.h>
#include <ActionGrantXP.h>
#include <ActionSetNPCState.h>
#include <ActionSpawn.h>
#include <ActionStartEvent.h>
#include <ActionUpdateFlag.h>
#include <ActionUpdateLNC.h>
#include <ActionUpdateQuest.h>
#include <ActionUpdateZoneFlags.h>
#include <ActionZoneChange.h>
#include <MiSpotData.h>
#include <ObjectPosition.h>
#include <Party.h>
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
    mActionHandlers[objects::Action::ActionType_t::SET_NPC_STATE] =
        &ActionManager::SetNPCState;
    mActionHandlers[objects::Action::ActionType_t::ADD_REMOVE_ITEMS] =
        &ActionManager::AddRemoveItems;
    mActionHandlers[objects::Action::ActionType_t::GRANT_XP] =
        &ActionManager::GrantXP;
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

bool ActionManager::StartEvent(const ActionContext& ctx)
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

bool ActionManager::ZoneChange(const ActionContext& ctx)
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
    float x = act->GetDestinationX();
    float y = act->GetDestinationY();
    float rotation = act->GetDestinationRotation();

    uint32_t spotID = act->GetSpotID();
    if(spotID > 0)
    {
        // If a spot is specified, get a random point in that spot instead
        auto definitionManager = server->GetDefinitionManager();
        auto serverDataManager = server->GetServerDataManager();
        auto zoneDef = serverDataManager->GetZoneData(zoneID);

        uint32_t dynamicMapID = zoneDef ? zoneDef->GetDynamicMapID() : 0;
        auto spots = definitionManager->GetSpotData(dynamicMapID);
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
    if(!zoneManager->EnterZone(ctx.Client, zoneID, x, y, rotation, true))
    {
        LOG_ERROR(libcomp::String("Failed to add client to zone"
            " %1. Closing the connection.\n").Arg(zoneID));

        ctx.Client->Close();

        return false;
    }
    
    return true;
}

bool ActionManager::AddRemoveItems(const ActionContext& ctx)
{
    if(!ctx.Client)
    {
        LOG_ERROR("Attempted to add or remove items with no associated"
            " client connection\n");
        return false;
    }

    auto act = std::dynamic_pointer_cast<objects::ActionAddRemoveItems>(ctx.Action);

    auto characterManager = mServer.lock()->GetCharacterManager();
    auto state = ctx.Client->GetClientState();
    auto cState = state->GetCharacterState();
    auto items = act->GetItems();

    if(!characterManager->AddRemoveItems(ctx.Client, items, act->GetAdd()))
    {
        return !act->GetStopOnFailure();
    }

    return true;
}

bool ActionManager::GrantXP(const ActionContext& ctx)
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
    if(act->GetIncludeCharacter())
    {
        entityStates.push_back(state->GetCharacterState());
    }

    if(act->GetIncludePartner())
    {
        entityStates.push_back(state->GetDemonState());
    }

    for(auto eState : entityStates)
    {
        if(eState->Ready())
        {
            characterManager->ExperienceGain(ctx.Client, (uint64_t)act->GetXP(),
                eState->GetEntityID());
        }
    }

    return true;
}

bool ActionManager::SetNPCState(const ActionContext& ctx)
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
    else if(act->GetActorID() < 0)
    {
        oNPCState = ctx.CurrentZone->GetServerObject(ctx.SourceEntityID);
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

bool ActionManager::UpdateFlag(const ActionContext& ctx)
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

bool ActionManager::UpdateLNC(const ActionContext& ctx)
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

bool ActionManager::UpdateQuest(const ActionContext& ctx)
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

    eventManager->UpdateQuest(ctx.Client, act->GetQuestID(), act->GetPhase(), false,
        act->GetFlagStates());

    return true;
}

bool ActionManager::UpdateZoneFlags(const ActionContext& ctx)
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
                if(ctx.CurrentZone->GetFlagState(pair.first, val))
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

bool ActionManager::Spawn(const ActionContext& ctx)
{
    auto act = std::dynamic_pointer_cast<objects::ActionSpawn>(ctx.Action);
    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();

    std::set<uint32_t> groupIDs;
    for(uint32_t groupID : act->GetSpawnGroupIDs())
    {
        switch(act->GetConditions())
        {
        case objects::ActionSpawn::Conditions_t::ONE_TIME:
            if(!ctx.CurrentZone->GroupHasSpawned(groupID, false))
            {
                groupIDs.insert(groupID);
            }
            break;
        case objects::ActionSpawn::Conditions_t::NONE_EXIST:
            if(!ctx.CurrentZone->GroupHasSpawned(groupID, true))
            {
                groupIDs.insert(groupID);
            }
            break;
        case objects::ActionSpawn::Conditions_t::NONE:
        default:
            groupIDs.insert(groupID);
            break;
        }
    }

    if(groupIDs.size() > 0)
    {
        zoneManager->UpdateSpawnGroups(ctx.CurrentZone, true, 0, groupIDs);

        return true;
    }
    else
    {
        return false;
    }
}

bool ActionManager::CreateLoot(const ActionContext& ctx)
{
    auto act = std::dynamic_pointer_cast<objects::ActionCreateLoot>(ctx.Action);
    auto server = mServer.lock();
    auto characterManager = server->GetCharacterManager();
    auto zoneManager = server->GetZoneManager();

    std::list<std::shared_ptr<objects::ObjectPosition>> locations;
    switch(act->GetPosition())
    {
    case objects::ActionCreateLoot::Position_t::ABS:
        locations = act->GetLocations();
        break;
    case objects::ActionCreateLoot::Position_t::SOURCE_RELATIVE:
        {
            auto source = ctx.CurrentZone->GetEntity(ctx.SourceEntityID);
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
        characterManager->CreateLootFromDrops(lBox, drops, 0, true);

        auto lState = std::make_shared<LootBoxState>(lBox);
        lState->SetCurrentX(loc->GetX());
        lState->SetCurrentY(loc->GetY());
        lState->SetCurrentRotation(loc->GetRotation());
        lState->SetEntityID(server->GetNextEntityID());
        entityIDs.push_back(lState->GetEntityID());

        ctx.CurrentZone->AddLootBox(lState);

        if(firstClient)
        {
            zoneManager->SendLootBoxData(firstClient, lState, nullptr,
                true, true);
        }
    }
    
    if(lootTime != 0)
    {
        zoneManager->ScheduleEntityRemoval(lootTime, ctx.CurrentZone,
            entityIDs);
    }

    ChannelClientConnection::FlushAllOutgoing(zConnections);

    return true;
}
