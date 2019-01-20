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

#include "ActionManager.h"

// libcomp Includes
#include <DefinitionManager.h>
#include <Log.h>
#include <PacketCodes.h>
#include <Randomizer.h>
#include <ServerDataManager.h>

// Standard C++11 Includes
#include <math.h>

// object Includes
#include <Account.h>
#include <AccountLogin.h>
#include <AccountWorldData.h>
#include <Action.h>
#include <ActionAddRemoveItems.h>
#include <ActionAddRemoveStatus.h>
#include <ActionCreateLoot.h>
#include <ActionDelay.h>
#include <ActionDisplayMessage.h>
#include <ActionGrantSkills.h>
#include <ActionGrantXP.h>
#include <ActionPlayBGM.h>
#include <ActionPlaySoundEffect.h>
#include <ActionRunScript.h>
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
#include <ActionZoneInstance.h>
#include <ActionZoneChange.h>
#include <CharacterProgress.h>
#include <DemonBox.h>
#include <DestinyBox.h>
#include <DigitalizeState.h>
#include <DropSet.h>
#include <Expertise.h>
#include <EventInstance.h>
#include <EventState.h>
#include <InstanceAccess.h>
#include <Loot.h>
#include <LootBox.h>
#include <Match.h>
#include <MiCategoryData.h>
#include <MiItemData.h>
#include <MiPossessionData.h>
#include <MiSkillItemStatusCommonData.h>
#include <MiSpotData.h>
#include <MiStatusData.h>
#include <MiTimeLimitData.h>
#include <MiZoneData.h>
#include <ObjectPosition.h>
#include <Party.h>
#include <PentalphaEntry.h>
#include <PentalphaMatch.h>
#include <PostItem.h>
#include <PvPInstanceStats.h>
#include <PvPMatch.h>
#include <Quest.h>
#include <ServerNPC.h>
#include <ServerObject.h>
#include <ServerZone.h>
#include <ServerZoneInstance.h>
#include <ServerZoneTrigger.h>
#include <Team.h>
#include <WorldSharedConfig.h>

// channel Includes
#include "AccountManager.h"
#include "ChannelServer.h"
#include "ChannelSyncManager.h"
#include "CharacterManager.h"
#include "EventManager.h"
#include "ManagerConnection.h"
#include "MatchManager.h"
#include "TokuseiManager.h"
#include "ZoneManager.h"

using namespace channel;

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
    mActionHandlers[objects::Action::ActionType_t::ADD_REMOVE_STATUS] =
        &ActionManager::AddRemoveStatus;
    mActionHandlers[objects::Action::ActionType_t::UPDATE_COMP] =
        &ActionManager::UpdateCOMP;
    mActionHandlers[objects::Action::ActionType_t::GRANT_SKILLS] =
        &ActionManager::GrantSkills;
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
    mActionHandlers[objects::Action::ActionType_t::UPDATE_POINTS] =
        &ActionManager::UpdatePoints;
    mActionHandlers[objects::Action::ActionType_t::UPDATE_QUEST] =
        &ActionManager::UpdateQuest;
    mActionHandlers[objects::Action::ActionType_t::UPDATE_ZONE_FLAGS] =
        &ActionManager::UpdateZoneFlags;
    mActionHandlers[objects::Action::ActionType_t::ZONE_INSTANCE] =
        &ActionManager::UpdateZoneInstance;
    mActionHandlers[objects::Action::ActionType_t::SPAWN] =
        &ActionManager::Spawn;
    mActionHandlers[objects::Action::ActionType_t::CREATE_LOOT] =
        &ActionManager::CreateLoot;
    mActionHandlers[objects::Action::ActionType_t::DELAY] =
        &ActionManager::Delay;
    mActionHandlers[objects::Action::ActionType_t::RUN_SCRIPT] =
        &ActionManager::RunScript;
}

ActionManager::~ActionManager()
{
}

void ActionManager::PerformActions(
    const std::shared_ptr<ChannelClientConnection>& client,
    const std::list<std::shared_ptr<objects::Action>>& actions,
    int32_t sourceEntityID, const std::shared_ptr<Zone>& zone,
    ActionOptions options)
{
    ActionContext ctx;
    ctx.Client = client;
    ctx.SourceEntityID = sourceEntityID;
    ctx.Options = options;

    if(zone)
    {
        ctx.CurrentZone = zone;
    }
    else if(ctx.Client)
    {
        ctx.CurrentZone = mServer.lock()->GetZoneManager()
            ->GetCurrentZone(ctx.Client);
    }

    if(!ctx.Client && ctx.CurrentZone && sourceEntityID)
    {
        // Add the client of the source if they are still in the zone
        auto sourceClient = mServer.lock()->GetManagerConnection()
            ->GetEntityClient(sourceEntityID);
        auto sourceState = sourceClient
            ? sourceClient->GetClientState() : nullptr;
        if(sourceState && sourceState->GetZone() == ctx.CurrentZone)
        {
            ctx.Client = sourceClient;
        }
    }

    for(auto action : actions)
    {
        if(ctx.ChannelChanged)
        {
            if(action->GetSourceContext() !=
                objects::Action::SourceContext_t::SOURCE)
            {
                LOG_ERROR(libcomp::String("Non-source context encountered"
                    " for action set that resulted in a channel change: %1\n")
                    .Arg(ctx.Client->GetClientState()
                        ->GetAccountUID().ToString()));
            }

            continue;
        }

        ctx.Action = action;

        auto it = mActionHandlers.find(action->GetActionType());

        if(mActionHandlers.end() == it)
        {
            LOG_ERROR(libcomp::String("Failed to parse action of type %1\n"
                ).Arg(to_underlying(action->GetActionType())));
            continue;
        }
        else
        {
            bool failure = false;

            auto srcCtx = action->GetSourceContext();
            if(srcCtx == objects::Action::SourceContext_t::ENEMIES)
            {
                // Execute once per enemy in the zone or instance and quit
                // afterwards if any fail
                std::list<std::shared_ptr<Zone>> zones;
                switch(action->GetLocation())
                {
                case objects::Action::Location_t::INSTANCE:
                    {
                        auto instance = ctx.CurrentZone->GetInstance();
                        if(instance)
                        {
                            zones = instance->GetZones();
                        }
                    }
                    break;
                case objects::Action::Location_t::ZONE:
                default:
                    // All others should be treated like just the zone
                    zones.push_back(ctx.CurrentZone);
                    break;
                }

                for(auto z : zones)
                {
                    // Include all enemy base entities (so allies too)
                    for(auto eBase : z->GetEnemiesAndAllies())
                    {
                        ActionContext copyCtx = ctx;
                        copyCtx.Client = nullptr;
                        copyCtx.SourceEntityID = eBase->GetEntityID();
                        copyCtx.Options.AutoEventsOnly = true;

                        failure |= !it->second(*this, copyCtx);
                    }
                }
            }
            else if(srcCtx == objects::Action::SourceContext_t::NONE)
            {
                // Remove current player context
                ActionContext copyCtx = ctx;
                copyCtx.Client = nullptr;
                copyCtx.SourceEntityID = 0;
                copyCtx.Options.AutoEventsOnly = true;

                failure |= !it->second(*this, copyCtx);
            }
            else if(srcCtx != objects::Action::SourceContext_t::SOURCE)
            {
                auto connectionManager = mServer.lock()->GetManagerConnection();

                // Execute once per source context character and quit afterwards
                // if any fail
                bool preFiltered = false;

                auto worldCIDs = GetActionContextCIDs(action, ctx, failure,
                    preFiltered);
                if(!failure)
                {
                    std::list<std::shared_ptr<CharacterState>> cStates;
                    for(auto worldCID : worldCIDs)
                    {
                        auto state = ClientState::GetEntityClientState(
                            worldCID, true);
                        if(state)
                        {
                            cStates.push_back(state->GetCharacterState());
                        }
                    }

                    if(!preFiltered)
                    {
                        auto ctxZone = ctx.CurrentZone;
                        auto ctxInst = ctxZone ? ctxZone->GetInstance() : nullptr;
                        switch(action->GetLocation())
                        {
                        case objects::Action::Location_t::INSTANCE:
                            cStates.remove_if([ctxInst](
                                const std::shared_ptr<CharacterState>& cState)
                                {
                                    auto z = cState->GetZone();
                                    return z == nullptr ||
                                        z->GetInstance() != ctxInst;
                                });
                            break;
                        case objects::Action::Location_t::ZONE:
                            cStates.remove_if([ctxZone](
                                const std::shared_ptr<CharacterState>& cState)
                                {
                                    return cState->GetZone() != ctxZone;
                                });
                            break;
                        case objects::Action::Location_t::CHANNEL:
                        default:
                            // No additional filtering
                            break;
                        }
                    }

                    // Now that the list is filtered, execute the actions
                    for(auto cState : cStates)
                    {
                        auto charClient = connectionManager->
                            GetEntityClient(cState->GetEntityID(), false);
                        if(charClient)
                        {
                            ActionContext copyCtx = ctx;
                            copyCtx.Client = charClient;
                            copyCtx.SourceEntityID = cState->GetEntityID();

                            // Auto-events only setting only applies to direct
                            // execution context
                            copyCtx.Options.AutoEventsOnly = false;

                            failure |= !it->second(*this, copyCtx);
                        }
                    }
                }
            }
            else
            {
                failure = !it->second(*this, ctx);

                if(ctx.Client)
                {
                    auto state = ctx.Client->GetClientState();
                    if(options.IncrementEventIndex)
                    {
                        auto current = state->GetEventState()->GetCurrent();
                        if(current)
                        {
                            current->SetIndex((uint16_t)(
                                current->GetIndex() + 1));
                        }
                    }
                }
            }

            if(failure && action->GetStopOnFailure())
            {
                if(!action->GetOnFailureEvent().IsEmpty())
                {
                    mServer.lock()->GetEventManager()->HandleEvent(ctx.Client,
                        action->GetOnFailureEvent(), ctx.SourceEntityID);
                }
                else
                {
                    LOG_DEBUG(libcomp::String("Quitting mid-action execution"
                        " following the result of action type: %1.\n")
                        .Arg((int32_t)action->GetActionType()));
                }

                break;
            }
        }
    }
}

void ActionManager::SendStageEffect(
    const std::shared_ptr<ChannelClientConnection>& client,
    int32_t messageID, int8_t effectType, bool includeMessage,
    int32_t messageValue)
{
    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_STAGE_EFFECT);
    p.WriteS32Little(messageID);
    p.WriteS8(effectType);

    bool valueSet = messageValue != 0;
    p.WriteS8(valueSet ? 1 : 0);
    if(valueSet)
    {
        p.WriteS32Little(messageValue);
    }

    client->QueuePacket(p);

    if(includeMessage)
    {
        p.Clear();
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_MESSAGE);
        p.WriteS32Little(messageID);

        client->QueuePacket(p);
    }

    client->FlushOutgoing();
}

std::set<int32_t> ActionManager::GetActionContextCIDs(
    const std::shared_ptr<objects::Action>& action, ActionContext& ctx,
    bool& failure, bool& preFiltered)
{
    auto connectionManager = mServer.lock()->GetManagerConnection();

    std::set<int32_t> worldCIDs;
    switch(action->GetSourceContext())
    {
    case objects::Action::SourceContext_t::ALL:
        // Sub-divide by location
        switch(action->GetLocation())
        {
        case objects::Action::Location_t::INSTANCE:
            if(ctx.CurrentZone)
            {
                std::list<std::shared_ptr<Zone>> zones;
                auto instance = ctx.CurrentZone->GetInstance();
                if(instance)
                {
                    zones = instance->GetZones();
                }

                for(auto z : zones)
                {
                    for(auto conn : z->GetConnectionList())
                    {
                        auto state = conn->GetClientState();
                        if(state)
                        {
                            worldCIDs.insert(state->GetWorldCID());
                        }
                    }
                }
            }
            break;
        case objects::Action::Location_t::ZONE:
            if(ctx.CurrentZone)
            {
                for(auto conn : ctx.CurrentZone->GetConnectionList())
                {
                    auto state = conn->GetClientState();
                    if(state)
                    {
                        worldCIDs.insert(state->GetWorldCID());
                    }
                }
            }
            break;
        case objects::Action::Location_t::CHANNEL:
            for(auto conn : connectionManager->GetAllConnections())
            {
                auto state = conn->GetClientState();
                if(state)
                {
                    worldCIDs.insert(state->GetWorldCID());
                }
            }
            break;
        default:
            // Not supported
            failure = true;
            break;
        }

        preFiltered = true;
        break;
    case objects::Action::SourceContext_t::PARTY:
    case objects::Action::SourceContext_t::TEAM:
        {
            auto sourceClient = ctx.Client
                ? ctx.Client : connectionManager->GetEntityClient(
                    ctx.SourceEntityID, false);
            if(!sourceClient)
            {
                failure = true;
                break;
            }

            auto state = sourceClient->GetClientState();
            switch(action->GetSourceContext())
            {
            case objects::Action::SourceContext_t::PARTY:
                {
                    auto party = state->GetParty();
                    if(party)
                    {
                        worldCIDs = party->GetMemberIDs();
                    }
                }
                break;
            case objects::Action::SourceContext_t::TEAM:
                {
                    auto team = state->GetTeam();
                    if(team)
                    {
                        worldCIDs = team->GetMemberIDs();
                    }
                }
                break;
            default:
                break;
            }

            // Always include self in group
            worldCIDs.insert(state->GetWorldCID());
        }
        break;
    case objects::Action::SourceContext_t::MATCH_TEAM:
        {
            auto sourceClient = ctx.Client
                ? ctx.Client : connectionManager->GetEntityClient(
                    ctx.SourceEntityID, false);
            if(!sourceClient)
            {
                failure = true;
                break;
            }

            auto state = sourceClient->GetClientState();

            auto match = ctx.CurrentZone
                ? ctx.CurrentZone->GetMatch() : nullptr;
            if(match && match->MemberIDsContains(state
                ->GetWorldCID()))
            {
                auto pvpMatch = std::dynamic_pointer_cast<
                    objects::PvPMatch>(match);
                if(pvpMatch)
                {
                    // Add PvP team participants
                    for(auto& team : { pvpMatch->GetBlueMemberIDs(),
                        pvpMatch->GetRedMemberIDs() })
                    {
                        bool inTeam = false;
                        for(int32_t worldCID : team)
                        {
                            if(worldCID == state->GetWorldCID())
                            {
                                inTeam = true;
                            }

                            worldCIDs.insert(worldCID);
                        }

                        if(inTeam)
                        {
                            break;
                        }

                        worldCIDs.clear();
                    }
                }
                else
                {
                    // Add all participants
                    worldCIDs = match->GetMemberIDs();
                }
            }
            else
            {
                failure = true;
                break;
            }
        }
        break;
    default:
        break;
    }

    return worldCIDs;
}

bool ActionManager::StartEvent(ActionContext& ctx)
{
    auto act = GetAction<objects::ActionStartEvent>(ctx, false);
    if(!act)
    {
        return false;
    }

    auto server = mServer.lock();
    auto eventManager = server->GetEventManager();

    EventOptions options;
    options.ActionGroupID = ctx.Options.GroupID;
    options.AutoOnly = ctx.Options.AutoEventsOnly;
    options.NoInterrupt = ctx.Options.NoEventInterrupt;

    switch(act->GetAllowInterrupt())
    {
    case objects::ActionStartEvent::AllowInterrupt_t::YES:
        options.NoInterrupt = false;
        break;
    case objects::ActionStartEvent::AllowInterrupt_t::NO:
        options.NoInterrupt = true;
        break;
    default:
        break;
    }

    eventManager->HandleEvent(ctx.Client, act->GetEventID(),
        ctx.SourceEntityID, ctx.CurrentZone, options);
    
    return true;
}

bool ActionManager::ZoneChange(ActionContext& ctx)
{
    auto act = GetAction<objects::ActionZoneChange>(ctx, true);
    if(!act)
    {
        return false;
    }

    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();

    // Where is the character going?
    uint32_t zoneID = act->GetZoneID();
    uint32_t dynamicMapID = act->GetDynamicMapID();
    float x = act->GetDestinationX();
    float y = act->GetDestinationY();
    float rotation = act->GetDestinationRotation();

    auto currentInstance = ctx.CurrentZone->GetInstance();

    bool moveAndQuit = false;

    uint32_t spotID = act->GetSpotID();
    if(zoneID && !dynamicMapID && currentInstance)
    {
        // Get the dynamic map ID from the instance
        auto instDef = currentInstance->GetDefinition();
        for(size_t i = 0; i < instDef->ZoneIDsCount(); i++)
        {
            if(instDef->GetZoneIDs(i) == zoneID)
            {
                dynamicMapID = instDef->GetDynamicMapIDs(i);
                break;
            }
        }
    }
    else if(!zoneID)
    {
        auto state = ctx.Client->GetClientState();
        auto cState = state->GetCharacterState();
        if(!spotID)
        {
            // Spot 0, zone 0 is a request to go to the homepoint
            auto character = cState->GetEntity();
            zoneID = character ? character->GetHomepointZone() : 0;
            spotID = character ? character->GetHomepointSpotID() : 0;

            if(!zoneID)
            {
                LOG_ERROR("Attempted to move to the homepoint but no"
                    " homepoint is set\n");
                return false;
            }
        }
        else if(cState->GetDisplayState() <= ActiveDisplayState_t::DATA_SENT)
        {
            // If we request a move before the character is even active,
            // just move the character and demon
            moveAndQuit = true;
        }
    }

    if(spotID > 0)
    {
        // If a spot is specified, get a random point in that spot instead
        std::shared_ptr<objects::ServerZone> zoneDef;

        if(zoneID == 0)
        {
            // Request is actually to move within the zone
            zoneDef = ctx.CurrentZone->GetDefinition();
            zoneID = zoneDef->GetID();
            dynamicMapID = zoneDef->GetDynamicMapID();
        }
        else
        {
            auto serverDataManager = server->GetServerDataManager();
            zoneDef = serverDataManager->GetZoneData(zoneID, dynamicMapID);
        }

        if(zoneDef)
        {
            auto definitionManager = server->GetDefinitionManager();
            auto zoneData = definitionManager->GetZoneData(zoneDef->GetID());
            auto spots = definitionManager->GetSpotData(zoneDef->GetDynamicMapID());
            auto spotIter = spots.find(spotID);
            if(spotIter != spots.end())
            {
                Point p = zoneManager->GetRandomSpotPoint(spotIter->second,
                    zoneData);
                x = p.x;
                y = p.y;
                rotation = spotIter->second->GetRotation();
            }
        }
        else
        {
            LOG_ERROR(libcomp::String("Invalid zone requested for spot ID"
                " move %1 (%2), #3.\n").Arg(zoneID).Arg(dynamicMapID).Arg(spotID));
            return false;
        }
    }

    if(moveAndQuit)
    {
        auto state = ctx.Client->GetClientState();
        auto cState = state->GetCharacterState();
        auto dState = state->GetDemonState();

        for(auto eState : { std::dynamic_pointer_cast<ActiveEntityState>(cState),
            std::dynamic_pointer_cast<ActiveEntityState>(dState) })
        {
            eState->SetOriginX(x);
            eState->SetOriginY(y);
            eState->SetOriginRotation(rotation);
            eState->SetDestinationX(x);
            eState->SetDestinationY(y);
            eState->SetDestinationRotation(rotation);
            eState->SetCurrentX(x);
            eState->SetCurrentY(y);
            eState->SetCurrentRotation(rotation);
        }

        return true;
    }

    // Enter the new zone and always leave the old zone even if its the same.
    if(!zoneManager->EnterZone(ctx.Client, zoneID, dynamicMapID,
        x, y, rotation, true))
    {
        LOG_ERROR(libcomp::String("Failed to add client to zone"
            " %1 (%2).\n").Arg(zoneID).Arg(dynamicMapID));
        return false;
    }

    // Update to point to the new zone
    ctx.CurrentZone = zoneManager->GetCurrentZone(ctx.Client);
    ctx.ChannelChanged = !ctx.CurrentZone &&
        ctx.Client->GetClientState()->GetChannelLogin() != nullptr;

    return ctx.CurrentZone != nullptr || ctx.ChannelChanged;
}

bool ActionManager::SetHomepoint(ActionContext& ctx)
{
    auto act = GetAction<objects::ActionSetHomepoint>(ctx, true);
    if(!act)
    {
        return false;
    }
    
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
    auto act = GetAction<objects::ActionAddRemoveItems>(ctx, true);
    if(!act)
    {
        return false;
    }

    auto server = mServer.lock();
    auto characterManager = server->GetCharacterManager();
    auto definitionManager = server->GetDefinitionManager();
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
        else if(itemPair.second > 0 ||
            (act->GetFromDropSet() && itemPair.second == 0))
        {
            adds[itemPair.first] = (uint32_t)itemPair.second;
        }
    }

    if(act->GetFromDropSet())
    {
        // Keys are actually drop set IDs and values are the maximum number of
        // drops that can pull from the set, removes are not valid
        if(removes.size() > 0)
        {
            LOG_ERROR("Attempted to remove items via drop set based action\n");
            return false;
        }

        auto serverDataManager = server->GetServerDataManager();

        std::unordered_map<uint32_t, uint32_t> dropItems;
        for(auto& pair : adds)
        {
            auto dropSet = serverDataManager->GetDropSetData(pair.first);
            if(dropSet)
            {
                // Value of 0 does not require or limit the number of drops
                auto drops = characterManager->DetermineDrops(dropSet
                    ->GetDrops(), 0, pair.second != 0);
                auto loot = characterManager->CreateLootFromDrops(drops);

                // Limit drop count
                if(pair.second > 0)
                {
                    while(loot.size() > (size_t)pair.second)
                    {
                        loot.pop_back();
                    }
                }

                for(auto l : loot)
                {
                    if(dropItems.find(l->GetType()) != dropItems.end())
                    {
                        dropItems[l->GetType()] = (uint32_t)(
                            dropItems[l->GetType()] + l->GetCount());
                    }
                    else
                    {
                        dropItems[l->GetType()] = (uint32_t)l->GetCount();
                    }
                }
            }
        }

        adds = dropItems;
    }

    switch(act->GetMode())
    {
    case objects::ActionAddRemoveItems::Mode_t::INVENTORY:
    case objects::ActionAddRemoveItems::Mode_t::TIME_TRIAL_REWARD:
        {
            bool timeTrialReward = act->GetMode() ==
                objects::ActionAddRemoveItems::Mode_t::TIME_TRIAL_REWARD;
            if(timeTrialReward)
            {
                auto character = cState->GetEntity();
                auto progress = character ? character->GetProgress().Get()
                    : nullptr;
                if(!progress || progress->GetTimeTrialID() <= 0)
                {
                    LOG_ERROR(libcomp::String("Attempted to grant time trial"
                        " rewards when no complete time trial exists: %1\n")
                        .Arg(state->GetAccountUID().ToString()));
                    return false;
                }
            }

            if(!characterManager->AddRemoveItems(ctx.Client, adds, true) ||
                !characterManager->AddRemoveItems(ctx.Client, removes, false))
            {
                return false;
            }

            if(timeTrialReward)
            {
                // Typically only one reward is set per trial
                uint32_t rewardItem = adds.size() > 0
                    ? adds.begin()->first : 0;
                uint16_t rewardItemCount = adds.size() > 0
                    ? (uint16_t)adds.begin()->second : (uint16_t)0;

                RecordTimeTrial(ctx, rewardItem, rewardItemCount);
            }

            if(adds.size() > 0 && (act->GetNotify() || timeTrialReward))
            {
                libcomp::Packet p;
                p.WritePacketCode(
                    ChannelToClientPacketCode_t::PACKET_EVENT_GET_ITEMS);
                p.WriteS8((int8_t)adds.size());
                for(auto entry : adds)
                {
                    p.WriteU32Little(entry.first);              // Type
                    p.WriteU16Little((uint16_t)entry.second);   // Quantity
                }

                ctx.Client->QueuePacket(p);
            }

            ctx.Client->FlushOutgoing();
        }
        break;
    case objects::ActionAddRemoveItems::Mode_t::MATERIAL_TANK:
        {
            // Make sure we have valid materials first
            for(auto& itemSet : { adds, removes })
            {
                for(auto pair : itemSet)
                {
                    auto itemData = definitionManager->GetItemData(pair.first);
                    auto categoryData = itemData
                        ? itemData->GetCommon()->GetCategory() : nullptr;
                    if(!categoryData || categoryData->GetMainCategory() != 1 ||
                        categoryData->GetSubCategory() != 64)
                    {
                        LOG_ERROR(libcomp::String("Attempted to add or remove"
                            " non-material item in the material tank: %1\n")
                            .Arg(pair.first));
                        return false;
                    }
                }
            }

            auto character = cState->GetEntity();
            if(character)
            {
                auto materials = character->GetMaterials();

                std::set<uint32_t> updates;
                for(auto pair : adds)
                {
                    uint32_t itemType = pair.first;
                    auto itemData = definitionManager->GetItemData(pair.first);

                    int32_t maxStack = (int32_t)itemData->GetPossession()
                        ->GetStackSize();

                    auto it = materials.find(itemType);
                    int32_t newStack = (int32_t)((it != materials.end()
                        ? it->second : 0) + pair.second);

                    if(newStack > maxStack)
                    {
                        newStack = (int32_t)maxStack;
                    }

                    materials[itemType] = (uint16_t)newStack;

                    updates.insert(itemType);
                }

                for(auto pair : removes)
                {
                    uint32_t itemType = pair.first;

                    auto it = materials.find(itemType);
                    int32_t newStack = (int32_t)((it != materials.end()
                        ? it->second : 0) - pair.second);

                    if(newStack < 0)
                    {
                        // Not enough materials
                        return false;
                    }
                    else if(newStack == 0)
                    {
                        materials.erase(itemType);
                    }
                    else
                    {
                        materials[itemType] = (uint16_t)newStack;
                    }

                    updates.insert(itemType);
                }

                character->SetMaterials(materials);

                server->GetWorldDatabase()
                    ->QueueUpdate(character, state->GetAccountUID());

                characterManager->SendMaterials(ctx.Client, updates);
            }
            else
            {
                return false;
            }
        }
        break;
    case objects::ActionAddRemoveItems::Mode_t::POST:
        if(removes.size() > 0)
        {
            LOG_ERROR("Attempted to remove one or more items"
                " from a post which is not allowed.\n");
            return false;
        }
        else
        {
            // Make sure they're valid products first
            for(auto pair : adds)
            {
                auto product = definitionManager->GetShopProductData(pair.first);
                if(!product)
                {
                    LOG_ERROR(libcomp::String("Attempted to add an invalid"
                        " product to a post: %1\n").Arg(pair.first));
                    return false;
                }
            }

            auto lobbyDB = server->GetLobbyDatabase();

            auto postItems = objects::PostItem::LoadPostItemListByAccount(
                lobbyDB, state->GetAccountUID());

            auto dbChanges = libcomp::DatabaseChangeSet::Create();
            for(auto pair : adds)
            {
                for(uint32_t i = 0; i < pair.second; i++)
                {
                    if((postItems.size() + pair.second) >= MAX_POST_ITEM_COUNT)
                    {
                        return false;
                    }

                    auto postItem = libcomp::PersistentObject::New<
                        objects::PostItem>(true);
                    postItem->SetType(pair.first);
                    postItem->SetTimestamp((uint32_t)std::time(0));
                    postItem->SetAccount(state->GetAccountUID());

                    dbChanges->Insert(postItem);

                    postItems.push_back(postItem);
                }
            }

            if(!lobbyDB->ProcessChangeSet(dbChanges))
            {
                LOG_ERROR("Attempted to remove one or more items"
                    " from a post which is not allowed.\n");
                return false;
            }
        }
        break;
    case objects::ActionAddRemoveItems::Mode_t::CULTURE_PICKUP:
        return characterManager->CultureItemPickup(ctx.Client);
    case objects::ActionAddRemoveItems::Mode_t::DESTINY_BOX:
        // Generate loot from items and add to player's box or remove from
        // player's box and put in inventory
        {
            int32_t worldCID = ctx.Client->GetClientState()->GetWorldCID();
            auto instance = ctx.CurrentZone->GetInstance();
            auto dBox = instance ? instance->GetDestinyBox(worldCID) : nullptr;
            if(!dBox)
            {
                return false;
            }

            size_t boxSize = dBox->LootCount();

            // Removes are either clear requests (key 0) or requests to
            // move the inventory (1 based indexes) with a value specifying
            // how many sequential slots will be affected
            std::unordered_map<uint32_t, uint32_t> toInventory;
            std::set<uint8_t> removeSlots;
            for(auto& pair : removes)
            {
                bool removing = false;

                size_t startingSlot = 0;
                if(pair.first == 0)
                {
                    // Remove backwards starting at position before next
                    // and don't add to inventory
                    startingSlot = (size_t)((dBox->GetNextPosition() +
                        boxSize - 1) % boxSize);
                    removing = true;
                }
                else
                {
                    // Move to inventory (wrap if past max slot)
                    startingSlot = (size_t)((pair.first - 1) % boxSize);
                }

                size_t slot = startingSlot;
                for(uint32_t i = 0; i < pair.second && (i == 0 ||
                    slot != startingSlot); i++)
                {
                    auto l = dBox->GetLoot(slot);
                    if(l && !removing)
                    {
                        if(toInventory.find(l->GetType()) != toInventory.end())
                        {
                            toInventory[l->GetType()] = (uint32_t)(
                                toInventory[l->GetType()] + l->GetCount());
                        }
                        else
                        {
                            toInventory[l->GetType()] = (uint32_t)l->GetCount();
                        }
                    }

                    removeSlots.insert((uint8_t)slot);

                    if(removing)
                    {
                        if(slot == 0)
                        {
                            // Wrap back to the end
                            slot = (boxSize - 1);
                        }
                        else
                        {
                            slot--;
                        }
                    }
                    else
                    {
                        slot++;
                        if(slot >= boxSize)
                        {
                            // Wrap back to start
                            slot = 0;
                        }
                    }
                }
            }

            std::list<std::shared_ptr<objects::Loot>> loot;
            for(auto& pair : adds)
            {
                auto l = std::make_shared<objects::Loot>();
                l->SetType(pair.first);
                l->SetCount((uint16_t)pair.second);

                loot.push_back(l);
            }

            if(toInventory.size() > 0)
            {
                // Do not fail from running out of inventory space here
                characterManager->AddRemoveItems(ctx.Client, toInventory,
                    true);
            }

            // Adds must succeed
            return server->GetZoneManager()->UpdateDestinyBox(instance,
                worldCID, loot, removeSlots) || loot.size() == 0;
        }
        break;
    default:
        return false;
    }

    return true;
}

bool ActionManager::AddRemoveStatus(ActionContext& ctx)
{
    auto act = GetAction<objects::ActionAddRemoveStatus>(ctx, false);
    if(!act)
    {
        return false;
    }

    auto state = ctx.Client ? ctx.Client->GetClientState() : nullptr;
    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto tokuseiManager = server->GetTokuseiManager();

    StatusEffectChanges effects;
    for(auto pair : act->GetStatusStacks())
    {
        effects[pair.first] = StatusEffectChange(pair.first, pair.second,
            act->GetIsReplace());

        if(act->StatusTimesKeyExists(pair.first))
        {
            // Explicit time specified
            effects[pair.first].Duration = act->GetStatusTimes(pair.first);
        }
    }

    if(effects.size() > 0)
    {
        std::list<std::shared_ptr<ActiveEntityState>> entities;
        bool playerEntities = true;
        switch(act->GetTargetType())
        {
        case objects::ActionAddRemoveStatus::TargetType_t::CHARACTER:
            if(state)
            {
                entities.push_back(state->GetCharacterState());
            }
            break;
        case objects::ActionAddRemoveStatus::TargetType_t::PARTNER:
            if(state)
            {
                entities.push_back(state->GetDemonState());
            }
            break;
        case objects::ActionAddRemoveStatus::TargetType_t::CHARACTER_AND_PARTNER:
            if(state)
            {
                entities.push_back(state->GetCharacterState());
                entities.push_back(state->GetDemonState());
            }
            break;
        case objects::ActionAddRemoveStatus::TargetType_t::SOURCE:
            {
                auto eState = ctx.CurrentZone->GetActiveEntity(
                    ctx.SourceEntityID);
                if(eState)
                {
                    entities.push_back(eState);
                }

                playerEntities = false;
            }
            break;
        }

        bool allowNull = act->GetAllowNull() && server->GetWorldSharedConfig()
            ->GetNRAStatusNull();
        for(auto eState : entities)
        {
            if(allowNull)
            {
                // Copy the effects that are not NRA'd
                StatusEffectChanges activeEffects;
                for(auto& pair : effects)
                {
                    auto statusDef = definitionManager->GetStatusData(
                        pair.first);
                    if(statusDef)
                    {
                        uint8_t affinity = statusDef->GetCommon()
                            ->GetAffinity();
                        CorrectTbl nraType = (CorrectTbl)(affinity +
                            (uint8_t)CorrectTbl::NRA_DEFAULT);
                        if(eState->GetNRAChance(NRA_NULL, nraType) > 0 ||
                            eState->GetNRAChance(NRA_REFLECT, nraType) > 0 ||
                            eState->GetNRAChance(NRA_ABSORB, nraType) > 0)
                        {
                            // Nullified, do not add
                            continue;
                        }
                    }

                    activeEffects[pair.first] = pair.second;
                }

                eState->AddStatusEffects(activeEffects, definitionManager);
            }
            else
            {
                eState->AddStatusEffects(effects, definitionManager);
            }

            if(!playerEntities)
            {
                tokuseiManager->Recalculate(eState, true);
            }
        }

        if(playerEntities)
        {
            // Recalculate the character and demon
            std::set<int32_t> entityIDs;
            for(auto eState : entities)
            {
                entityIDs.insert(eState->GetEntityID());
            }

            tokuseiManager->Recalculate(state->GetCharacterState(), true,
                entityIDs);

            for(auto eState : entities)
            {
                server->GetCharacterManager()->RecalculateStats(eState,
                    ctx.Client);
            }
        }
    }

    return true;
}

bool ActionManager::UpdateCOMP(ActionContext& ctx)
{
    auto act = GetAction<objects::ActionUpdateCOMP>(ctx, true);
    if(!act)
    {
        return false;
    }

    auto server = mServer.lock();
    auto characterManager = server->GetCharacterManager();
    auto state = ctx.Client->GetClientState();
    auto cState = state->GetCharacterState();
    auto character = cState->GetEntity();
    auto dState = state->GetDemonState();
    auto progress = character->GetProgress().Get();
    auto comp = character->GetCOMP().Get();

    // Before updating the COMP values, perform unsummon if requested
    if(act->GetUnsummon())
    {
        characterManager->StoreDemon(ctx.Client);
    }

    // First increase the COMP
    uint8_t maxSlots = progress->GetMaxCOMPSlots();
    if(act->GetAddSlot() > 0)
    {
        maxSlots = (uint8_t)(maxSlots + act->GetAddSlot());
        if(maxSlots > 10)
        {
            maxSlots = 10;
        }
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
        if(act->RemoveDemonsKeyExists(0))
        {
            auto d = dState->GetEntity();
            if(d)
            {
                if(d->GetLocked())
                {
                    LOG_ERROR("Attempted to remove partner demon that"
                        " is locked\n");
                    return false;
                }
                else
                {
                    remove[0].push_back(d);
                }
            }
            else
            {
                LOG_ERROR("Attempted to remove partner demon but no"
                    " demon was summoned for COMP removal request\n");
                return false;
            }
        }

        for(uint8_t i = 0; i < maxSlots; i++)
        {
            auto slot = comp->GetDemons((size_t)i);
            if(!slot.IsNull() && !slot->GetLocked())
            {
                // If there are more than one specified, the ones near the
                // start of the COMP will be removed first
                uint32_t type = slot->GetType();
                if(act->RemoveDemonsKeyExists(type))
                {
                    if(act->GetRemoveDemons(type) == 0)
                    {
                        // Special case, must be summoned demon
                        if(remove.find(0) != remove.end())
                        {
                            LOG_ERROR("Attempted to remove partner demon twice"
                                " for COMP removal request\n");
                            return false;
                        }
                        else if(dState->GetEntity() == slot.Get())
                        {
                            remove[type].push_back(slot.Get());
                        }
                        else
                        {
                            LOG_ERROR("Attempted to remove specific partner demon that"
                                " was not summoned for COMP removal request\n");
                            return false;
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
                LOG_ERROR("One or more demons does not exist or is locked"
                    " for COMP removal request\n");
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

        libcomp::Packet p;
        p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_COMP_SIZE_UPDATED);
        p.WriteU8(maxSlots);

        ctx.Client->QueuePacket(p);
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

    ctx.Client->FlushOutgoing();

    return true;
}

bool ActionManager::GrantXP(ActionContext& ctx)
{
    auto act = GetAction<objects::ActionGrantXP>(ctx, true);
    if(!act)
    {
        return false;
    }

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
    auto act = GetAction<objects::ActionGrantSkills>(ctx, true);
    if(!act)
    {
        return false;
    }

    auto server = mServer.lock();
    auto characterManager = server->GetCharacterManager();
    auto state = ctx.Client->GetClientState();

    std::shared_ptr<ActiveEntityState> eState;
    switch(act->GetTargetType())
    {
    case objects::ActionGrantSkills::TargetType_t::CHARACTER:
        {
            auto character = state->GetCharacterState()->GetEntity();
            eState = state->GetCharacterState();

            if(act->GetSkillPoints() > 0)
            {
                characterManager->UpdateSkillPoints(ctx.Client,
                    act->GetSkillPoints());
            }

            if(act->ExpertisePointsCount() > 0)
            {
                bool expSet = act->GetExpertiseSet();

                std::list<std::pair<uint8_t, int32_t>> expPoints;
                for(auto pair : act->GetExpertisePoints())
                {
                    int32_t points = pair.second;
                    if(expSet)
                    {
                        // Explicitly set the points
                        auto exp = character->GetExpertises(
                            (size_t)pair.first).Get();
                        if(exp)
                        {
                            points = points - exp->GetPoints();
                        }
                    }

                    expPoints.push_back(std::pair<uint8_t, int32_t>(pair.first,
                        points));
                }

                characterManager->UpdateExpertisePoints(ctx.Client, expPoints);
            }

            if(act->GetExpertiseMax() > 0)
            {
                uint8_t val = act->GetExpertiseMax();

                int16_t newVal = (int16_t)(
                    character->GetExpertiseExtension() + val);
                if(newVal > 127)
                {
                    newVal = 127;
                }

                if((int8_t)newVal != character->GetExpertiseExtension())
                {
                    character->SetExpertiseExtension((int8_t)newVal);

                    characterManager->SendExpertiseExtension(ctx.Client);
                    server->GetWorldDatabase()->QueueUpdate(character,
                        state->GetAccountUID());
                }
            }
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

        if(act->GetExpertiseMax() > 0)
        {
            LOG_ERROR("Attempted to extend max expertise for a partner demon\n");
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
    auto act = GetAction<objects::ActionDisplayMessage>(ctx, true);
    if(!act)
    {
        return false;
    }

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
    auto act = GetAction<objects::ActionStageEffect>(ctx, true);
    if(!act)
    {
        return false;
    }

    SendStageEffect(ctx.Client, act->GetMessageID(), act->GetEffectType(),
        act->GetIncludeMessage(), act->GetMessageValue());

    return true;
}

bool ActionManager::SpecialDirection(ActionContext& ctx)
{
    auto act = GetAction<objects::ActionSpecialDirection>(ctx, true);
    if(!act)
    {
        return false;
    }

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
    auto act = GetAction<objects::ActionPlayBGM>(ctx, true);
    if(!act)
    {
        return false;
    }

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
    auto act = GetAction<objects::ActionPlaySoundEffect>(ctx, true);
    if(!act)
    {
        return false;
    }
    
    libcomp::Packet p;
    p.WritePacketCode(ChannelToClientPacketCode_t::PACKET_EVENT_PLAY_SOUND_EFFECT);
    p.WriteS32Little(act->GetSoundID());
    p.WriteS32Little(act->GetDelay());

    ctx.Client->SendPacket(p);

    return true;
}

bool ActionManager::SetNPCState(ActionContext& ctx)
{
    auto act = GetAction<objects::ActionSetNPCState>(ctx, false);
    if(!act)
    {
        return false;
    }
    else if(!ctx.Client && act->GetSourceClientOnly())
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
        LOG_ERROR(libcomp::String("SetNPCState attempted on invalid target: %1\n")
            .Arg(act->GetActorID()));
        return false;
    }

    std::shared_ptr<objects::ServerObject> oNPC;
    switch(oNPCState->GetEntityType())
    {
    case EntityType_t::NPC:
        oNPC = std::dynamic_pointer_cast<NPCState>(oNPCState)->GetEntity();
        break;
    case EntityType_t::OBJECT:
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

        uint8_t from = oNPC->GetState();
        if(!act->GetSourceClientOnly())
        {
            oNPC->SetState(act->GetState());
        }

        std::list<std::shared_ptr<ChannelClientConnection>> clients;
        if(act->GetSourceClientOnly())
        {
            clients.push_back(ctx.Client);
        }
        else
        {
            clients = ctx.CurrentZone->GetConnectionList();
        }

        auto npcState = std::dynamic_pointer_cast<NPCState>(oNPCState);
        if(npcState)
        {
            if(act->GetState() == 1)
            {
                zoneManager->ShowNPC(ctx.CurrentZone, clients, npcState,
                    false);
            }
            else
            {
                zoneManager->RemoveEntities(clients,
                    { npcState->GetEntityID() });
            }
        }
        else
        {
            if(!act->GetSourceClientOnly())
            {
                // Update collisions
                zoneManager->UpdateGeometryElement(ctx.CurrentZone, oNPC);
            }

            if(act->GetState() == 255)
            {
                zoneManager->RemoveEntities(clients,
                    { oNPCState->GetEntityID() });
            }
            else if(from == 255)
            {
                auto objState = std::dynamic_pointer_cast<ServerObjectState>(
                    oNPCState);
                zoneManager->ShowObject(ctx.CurrentZone, clients, objState,
                    false);
            }
            else
            {
                libcomp::Packet p;
                p.WritePacketCode(
                    ChannelToClientPacketCode_t::PACKET_NPC_STATE_CHANGE);
                p.WriteS32Little(oNPCState->GetEntityID());
                p.WriteU8(act->GetState());

                ChannelClientConnection::BroadcastPacket(clients, p);
            }
        }
    }

    return true;
}

bool ActionManager::UpdateFlag(ActionContext& ctx)
{
    auto act = GetAction<objects::ActionUpdateFlag>(ctx, true);
    if(!act)
    {
        return false;
    }

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
    case objects::ActionUpdateFlag::FlagType_t::TIME_TRIAL:
        return RecordTimeTrial(ctx, 0, 0);
        break;
    default:
        return false;
    }

    return true;
}

bool ActionManager::UpdateLNC(ActionContext& ctx)
{
    auto act = GetAction<objects::ActionUpdateLNC>(ctx, true);
    if(!act)
    {
        return false;
    }

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

bool ActionManager::UpdatePoints(ActionContext& ctx)
{
    auto act = GetAction<objects::ActionUpdatePoints>(ctx, false);
    if(!act)
    {
        return false;
    }

    if(!ctx.Client && act->GetPointType() !=
        objects::ActionUpdatePoints::PointType_t::KILL_VALUE)
    {
        LOG_ERROR("Attempted to set non-player entity points\n");
        return false;
    }

    switch(act->GetPointType())
    {
    case objects::ActionUpdatePoints::PointType_t::CP:
        if(act->GetIsSet() || act->GetValue() < 0)
        {
            LOG_ERROR("Attempts to explicitly set or decrease the player's CP"
                " are not allowed!\n");
            return false;
        }
        else
        {
            auto server = mServer.lock();

            auto account = ctx.Client->GetClientState()
                ->GetAccountLogin()->GetAccount().Get();
            
            auto accountManager = server->GetAccountManager();
            if(accountManager->IncreaseCP(account, act->GetValue()))
            {
                accountManager->SendCPBalance(ctx.Client);
            }
        }
        break;
    case objects::ActionUpdatePoints::PointType_t::DIGITALIZE_POINTS:
        {
            auto state = ctx.Client->GetClientState();
            auto cState = state->GetCharacterState();
            auto character = cState->GetEntity();
            auto progress = character
                ? character->GetProgress().Get() : nullptr;

            auto dgState = cState->GetDigitalizeState();
            if(!dgState)
            {
                return false;
            }

            int32_t points = (int32_t)act->GetValue();
            if(points < 0)
            {
                return false;
            }

            if(act->GetIsSet())
            {
                int32_t existing = progress->GetDigitalizePoints(
                    dgState->GetRaceID());
                if(existing > points)
                {
                    LOG_ERROR("Attempted to lower digitalize points with"
                        " direct set action\n");
                    return false;
                }

                points = points - existing;
            }

            std::unordered_map<uint8_t, int32_t> pointMap;
            pointMap[dgState->GetRaceID()] = points;

            if(!mServer.lock()->GetCharacterManager()->UpdateDigitalizePoints(
                ctx.Client, pointMap, !act->GetIsSet()))
            {
                return false;
            }
        }
        break;
    case objects::ActionUpdatePoints::PointType_t::SOUL_POINTS:
        {
            mServer.lock()->GetCharacterManager()->UpdateSoulPoints(
                ctx.Client, (int32_t)act->GetValue(), !act->GetIsSet());
        }
        break;
    case objects::ActionUpdatePoints::PointType_t::COINS:
        {
            mServer.lock()->GetCharacterManager()->UpdateCoinTotal(
                ctx.Client, act->GetValue(), !act->GetIsSet());
        }
        break;
    case objects::ActionUpdatePoints::PointType_t::ITIME:
        if(act->GetModifier() > 0)
        {
            auto state = ctx.Client->GetClientState();
            auto cState = state->GetCharacterState();
            auto character = cState->GetEntity();
            auto progress = character
                ? character->GetProgress().Get() : nullptr;
            if(progress)
            {
                int8_t iTimeID = act->GetModifier();
                int16_t oldVal = progress->GetITimePoints(iTimeID);
                int16_t val = (int16_t)act->GetValue();
                if(!act->GetIsSet())
                {
                    val = (int16_t)(oldVal + val);
                    if(val < 0)
                    {
                        // Value cannot become negative
                        val = 0;
                    }
                }

                if(oldVal != val)
                {
                    if(val >= 0)
                    {
                        // Set value normally
                        progress->SetITimePoints(iTimeID, val);
                    }
                    else
                    {
                        // Reset entry
                        progress->RemoveITimePoints(iTimeID);
                        val = 0;
                    }

                    libcomp::Packet p;
                    p.WritePacketCode(
                        ChannelToClientPacketCode_t::PACKET_ITIME_UPDATE);
                    p.WriteS8(iTimeID);
                    p.WriteS16Little(val);

                    ctx.Client->SendPacket(p);

                    mServer.lock()->GetWorldDatabase()->QueueUpdate(
                        progress, state->GetAccountUID());
                }
            }
            else
            {
                return false;
            }
        }
        else
        {
            LOG_ERROR("Invalid I-Time ID specified for UpdatePoints action\n");
            return false;
        }
        break;
    case objects::ActionUpdatePoints::PointType_t::BP:
        {
            mServer.lock()->GetCharacterManager()->UpdateBP(
                ctx.Client, (int32_t)act->GetValue(), !act->GetIsSet());
        }
        break;
    case objects::ActionUpdatePoints::PointType_t::KILL_VALUE:
        {
            auto eState = ctx.CurrentZone->GetActiveEntity(ctx.SourceEntityID);
            if(eState)
            {
                int32_t val = eState->GetKillValue();

                if(act->GetIsSet())
                {
                    val = (int32_t)act->GetValue();
                }
                else
                {
                    val += (int32_t)act->GetValue();
                }

                eState->SetKillValue(val);
            }
            else
            {
                return false;
            }
        }
        break;
    case objects::ActionUpdatePoints::PointType_t::PVP_POINTS:
        {
            auto state = ctx.Client->GetClientState();
            auto cState = state->GetCharacterState();

            auto instance = ctx.CurrentZone->GetInstance();
            if(!MatchManager::PvPActive(instance) || act->GetIsSet())
            {
                return false;
            }

            // Make sure the entity belongs to a PvP team faction group
            int32_t factionGroup = cState->GetFactionGroup();
            if(MatchManager::InPvPTeam(cState))
            {
                auto matchManager = mServer.lock()->GetMatchManager();
                if(!matchManager->UpdatePvPPoints(instance->GetID(),
                    cState->GetEntityID(), -1, (uint8_t)(factionGroup - 1),
                    (int32_t)act->GetValue(), false))
                {
                    return false;
                }
            }
        }
        break;
    case objects::ActionUpdatePoints::PointType_t::COWRIE:
        {
            return mServer.lock()->GetCharacterManager()->UpdateCowrieBethel(
                ctx.Client, (int32_t)act->GetValue(), { { 0, 0, 0, 0, 0 } });
        }
        break;
    case objects::ActionUpdatePoints::PointType_t::UB_POINTS:
        {
            return mServer.lock()->GetMatchManager()->UpdateUBPoints(
                ctx.Client, (int32_t)act->GetValue());
        }
        break;
    case objects::ActionUpdatePoints::PointType_t::BETHEL:
        {
            // Modifier required for team/bethel type specification
            auto server = mServer.lock();

            auto pEntry = server->GetMatchManager()->LoadPentalphaData(
                ctx.Client, 0x01);
            if(act->GetIsSet())
            {
                // Update points independent of entry (non-adjustable)
                std::array<int32_t, 5> bethel = { { 0, 0, 0, 0, 0 } };
                bethel[(size_t)act->GetModifier()] = (int32_t)act->GetValue();

                return server->GetCharacterManager()->UpdateCowrieBethel(
                    ctx.Client, 0, bethel);
            }
            else if(!pEntry)
            {
                // Everything past this point requires an active entry
                return false;
            }
            else
            {
                auto currentMatch = server->GetMatchManager()
                    ->GetPentalphaMatch(false);
                if(!currentMatch || pEntry->GetMatch() != currentMatch->GetUUID())
                {
                    // Not in the current match
                    return false;
                }

                auto zone = ctx.CurrentZone;
                auto instance = zone ? zone->GetInstance() : nullptr;

                int32_t oldBethel = pEntry->GetBethel();
                auto oldVals = pEntry->GetPoints();
                auto newVals = oldVals;
                if(act->GetValue())
                {
                    // Request to update points for current match (no removal)
                    int32_t bethel = (int32_t)act->GetValue();

                    pEntry->SetBethel(oldBethel + bethel);
                    newVals[pEntry->GetTeam()] = bethel +
                        oldVals[pEntry->GetTeam()];
                }
                else if(!instance ||
                    zone->GetInstanceType() != InstanceType_t::PENTALPHA)
                {
                    // Nothing to do
                    return true;
                }
                else
                {
                    // Request to pull all bethel from the client state, add
                    // the points to the current team and subtract from the
                    // team matching the instance sub ID
                    auto state = ctx.Client->GetClientState();
                    int32_t bethel = state->GetInstanceBethel();
                    size_t otherIdx = (size_t)instance->GetVariant()
                        ->GetSubID();

                    pEntry->SetBethel(oldBethel + bethel);
                    newVals[pEntry->GetTeam()] = oldVals[pEntry->GetTeam()] +
                        bethel;
                    newVals[otherIdx] = oldVals[otherIdx] - bethel;
                    state->SetInstanceBethel(0);

                    // Get the final amount
                    bethel = server->GetCharacterManager()
                        ->UpdateBethel(ctx.Client, bethel, true);

                    libcomp::Packet p;
                    p.WritePacketCode(
                        ChannelToClientPacketCode_t::PACKET_PENTALPHA_END);
                    p.WriteS32Little((int32_t)pEntry->GetTeam());
                    p.WriteS32Little(bethel);
                    p.WriteU32Little(0);   // Unknown

                    ctx.Client->SendPacket(p);
                }

                pEntry->SetPoints(newVals);
                if(pEntry->Update(server->GetWorldDatabase()))
                {
                    server->GetChannelSyncManager()->SyncRecordUpdate(pEntry,
                        "PentalphaEntry");
                }
                else
                {
                    // Rollback
                    pEntry->SetBethel(oldBethel);
                    pEntry->SetPoints(oldVals);
                    return false;
                }
            }
        }
        break;
    case objects::ActionUpdatePoints::PointType_t::ZIOTITE:
        {
            // Setting/increasing small ziotite by the value and large ziotite
            // by the modifier
            auto state = ctx.Client->GetClientState();
            auto team = state->GetTeam();
            if(!team)
            {
                // Not in a team
                return false;
            }

            int32_t sZiotite = (int32_t)act->GetValue();
            int8_t lZiotite = act->GetModifier();
            if(act->GetIsSet())
            {
                sZiotite = sZiotite - team->GetSmallZiotite();
                lZiotite = (int8_t)(lZiotite - team->GetLargeZiotite());
            }

            return mServer.lock()->GetMatchManager()->UpdateZiotite(team,
                sZiotite, lZiotite, state->GetWorldCID());
        }
        break;
    default:
        break;
    }

    return true;
}

bool ActionManager::UpdateQuest(ActionContext& ctx)
{
    auto act = GetAction<objects::ActionUpdateQuest>(ctx, true);
    if(!act)
    {
        return false;
    }

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

    return eventManager->UpdateQuest(ctx.Client, act->GetQuestID(), act->GetPhase(),
        act->GetForceUpdate(), flagStates);
}

bool ActionManager::UpdateZoneFlags(ActionContext& ctx)
{
    auto act = GetAction<objects::ActionUpdateZoneFlags>(ctx, false);
    if(!act)
    {
        return false;
    }

    // Determine if it affects the current character or the whole zone
    int32_t worldCID = 0;
    switch(act->GetType())
    {
    case objects::ActionUpdateZoneFlags::Type_t::ZONE_CHARACTER:
    case objects::ActionUpdateZoneFlags::Type_t::ZONE_INSTANCE_CHARACTER:
        if(ctx.Client)
        {
            worldCID = ctx.Client->GetClientState()->GetWorldCID();
        }
        else
        {
            LOG_ERROR("Attempted to update a zone character flag with"
                " no associated client!\n");
            return false;
        }
        break;
    default:
        break;
    }

    switch(act->GetType())
    {
    case objects::ActionUpdateZoneFlags::Type_t::ZONE:
    case objects::ActionUpdateZoneFlags::Type_t::ZONE_CHARACTER:
        switch(act->GetSetMode())
        {
        case objects::ActionUpdateZoneFlags::SetMode_t::UPDATE:
            for(auto pair : act->GetFlagStates())
            {
                ctx.CurrentZone->SetFlagState(pair.first, pair.second, worldCID);
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
                    if(!ctx.CurrentZone->GetFlagState(pair.first, val, worldCID))
                    {
                        val = 0;
                    }

                    val = val + (incr ? pair.second : -pair.second);
                    ctx.CurrentZone->SetFlagState(pair.first, val, worldCID);
                }
            }
            break;
        default:
            break;
        }
        break;
    case objects::ActionUpdateZoneFlags::Type_t::ZONE_INSTANCE:
    case objects::ActionUpdateZoneFlags::Type_t::ZONE_INSTANCE_CHARACTER:
        {
            auto instance = ctx.CurrentZone->GetInstance();
            if(instance == nullptr)
            {
                return false;
            }

            switch(act->GetSetMode())
            {
            case objects::ActionUpdateZoneFlags::SetMode_t::UPDATE:
                for(auto pair : act->GetFlagStates())
                {
                    instance->SetFlagState(pair.first, pair.second, worldCID);
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
                        if(!instance->GetFlagState(pair.first, val, worldCID))
                        {
                            val = 0;
                        }

                        val = val + (incr ? pair.second : -pair.second);
                        instance->SetFlagState(pair.first, val, worldCID);
                    }
                }
                break;
            default:
                break;
            }
        }
        break;
    case objects::ActionUpdateZoneFlags::Type_t::TOKUSEI:
    case objects::ActionUpdateZoneFlags::Type_t::PARTNER_TOKUSEI:
        // Set tokusei on the entity that clear when they leave the instance or
        // change global zones
        {
            auto eState = ctx.CurrentZone->GetActiveEntity(ctx.SourceEntityID);
            if(eState && act->GetType() ==
                objects::ActionUpdateZoneFlags::Type_t::PARTNER_TOKUSEI)
            {
                auto state = ClientState::GetEntityClientState(eState
                    ->GetEntityID());
                eState = state ? state->GetDemonState() : nullptr;
            }

            if(!eState)
            {
                // Entity is required
                return false;
            }

            switch(act->GetSetMode())
            {
            case objects::ActionUpdateZoneFlags::SetMode_t::UPDATE:
                for(auto pair : act->GetFlagStates())
                {
                    if(pair.second == 0)
                    {
                        eState->RemoveAdditionalTokusei(pair.first);
                    }
                    else if(pair.second > 0)
                    {
                        eState->SetAdditionalTokusei(pair.first,
                            (uint16_t)pair.second);
                    }
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
                        val = (int32_t)eState->GetAdditionalTokusei(pair.first);
                        val = val + (incr ? pair.second : -pair.second);
                        if(val <= 0)
                        {
                            eState->RemoveAdditionalTokusei(pair.first);
                        }
                        else
                        {
                            eState->SetAdditionalTokusei(pair.first,
                                (uint16_t)val);
                        }
                    }
                }
                break;
            default:
                break;
            }

            // If the entity is a partner demon, calculate tokusei from the
            // character instead
            if(eState->GetEntityType() == EntityType_t::PARTNER_DEMON)
            {
                auto state = ClientState::GetEntityClientState(
                    eState->GetEntityID());
                if(state)
                {
                    eState = state->GetCharacterState();
                }
            }

            mServer.lock()->GetTokuseiManager()->Recalculate(eState, true);
        }
        break;
    }

    if(act->GetType() == objects::ActionUpdateZoneFlags::Type_t::ZONE &&
        ctx.CurrentZone->FlagSetKeysCount() > 0)
    {
        // Check if any flags that have been set have action triggers
        std::set<int32_t> triggerFlags;
        for(auto pair : act->GetFlagStates())
        {
            if(ctx.CurrentZone->FlagSetKeysContains(pair.first))
            {
                triggerFlags.insert(pair.first);
            }
        }

        for(int32_t triggerFlag : triggerFlags)
        {
            for(auto trigger : ctx.CurrentZone->GetFlagSetTriggers())
            {
                if(trigger->GetValue() == triggerFlag)
                {
                    PerformActions(ctx.Client, trigger->GetActions(),
                        ctx.SourceEntityID, ctx.CurrentZone);
                }
            }
        }
    }

    return true;
}

bool ActionManager::UpdateZoneInstance(ActionContext& ctx)
{
    auto act = GetAction<objects::ActionZoneInstance>(ctx, false);
    if(!act)
    {
        return false;
    }

    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();

    switch(act->GetMode())
    {
    case objects::ActionZoneInstance::Mode_t::CREATE:
    case objects::ActionZoneInstance::Mode_t::SOLO_CREATE:
    case objects::ActionZoneInstance::Mode_t::TEAM_JOIN:
    case objects::ActionZoneInstance::Mode_t::CLAN_JOIN:
        if(!ctx.Client)
        {
            LOG_ERROR("Attempted to create an instance with no client"
                " context\n");
            return false;
        }
        else
        {
            auto serverDataManager = server->GetServerDataManager();
            auto instDef = serverDataManager->GetZoneInstanceData(
                act->GetInstanceID());
            if(!instDef)
            {
                LOG_ERROR(libcomp::String("Invalid zone instance ID could not"
                    " be created: %1\n").Arg(act->GetInstanceID()));
                return false;
            }

            auto state = ctx.Client->GetClientState();

            bool moveNow = false;
            std::set<int32_t> accessCIDs = { state->GetWorldCID() };
            switch(act->GetMode())
            {
            case objects::ActionZoneInstance::Mode_t::TEAM_JOIN:
                // Grant access to the team in the zone and join right away
                {
                    auto team = state->GetTeam();
                    if(team)
                    {
                        accessCIDs = team->GetMemberIDs();
                        accessCIDs.insert(state->GetWorldCID());
                    }

                    moveNow = true;
                }
                break;
            case objects::ActionZoneInstance::Mode_t::CLAN_JOIN:
                // Grant access to the clan in the zone and join right away
                {
                    auto character = state->GetCharacterState()->GetEntity();
                    auto clanUID = character
                        ? character->GetClan().GetUUID() : NULLUUID;
                    if(clanUID != NULLUUID)
                    {
                        for(auto& cPair : ctx.CurrentZone->GetConnections())
                        {
                            auto oState = cPair.second->GetClientState();
                            character = oState->GetCharacterState()
                                ->GetEntity();
                            if(character &&
                                character->GetClan().GetUUID() == clanUID)
                            {
                                accessCIDs.insert(oState->GetWorldCID());
                            }
                        }
                    }

                    moveNow = true;
                }
                break;
            case objects::ActionZoneInstance::Mode_t::SOLO_CREATE:
                // Only grant access to the client
                accessCIDs.insert(state->GetWorldCID());
                break;
            case objects::ActionZoneInstance::Mode_t::CREATE:
            default:
                // Grant access to all party members (or self if no party)
                {
                    auto party = state->GetParty();
                    if(party)
                    {
                        accessCIDs = party->GetMemberIDs();
                        accessCIDs.insert(state->GetWorldCID());
                    }
                }
                break;
            }

            if(moveNow)
            {
                // Filter down to just characters in the zone
                std::set<int32_t> remove;
                for(int32_t cid : accessCIDs)
                {
                    auto otherState = ClientState::GetEntityClientState(cid,
                        true);
                    if(cid != state->GetWorldCID() && (!otherState ||
                        otherState->GetZone() != ctx.CurrentZone))
                    {
                        remove.insert(cid);
                    }
                }

                for(int32_t cid : remove)
                {
                    accessCIDs.erase(cid);
                }
            }

            auto instAccess = std::make_shared<objects::InstanceAccess>();
            instAccess->SetAccessCIDs(accessCIDs);
            instAccess->SetDefinitionID(act->GetInstanceID());
            instAccess->SetVariantID(act->GetVariantID());
            instAccess->SetCreateTimerID(act->GetTimerID());
            instAccess->SetCreateTimerExpirationEventID(act
                ->GetTimerExpirationEventID());

            uint8_t resultCode = zoneManager->CreateInstance(instAccess);
            if(resultCode && moveNow)
            {
                // Move all players, kicking all players not in the source
                // player's team or not in the set from their current teams
                auto team = state->GetTeam();
                auto managerConnection = server->GetManagerConnection();
                auto matchManager = server->GetMatchManager();
                auto clients = managerConnection->GetEntityClients(accessCIDs,
                    true);
                for(auto client : clients)
                {
                    auto oState = client->GetClientState();
                    auto t = oState->GetTeam();
                    if(t && t != team)
                    {
                        matchManager->LeaveTeam(client, t->GetID());
                    }
                }

                if(team)
                {
                    for(auto client : managerConnection->GetEntityClients(team
                        ->GetMemberIDs(), true))
                    {
                        auto oState = client->GetClientState();
                        if(accessCIDs.find(oState->GetWorldCID()) ==
                            accessCIDs.end())
                        {
                            matchManager->LeaveTeam(client, team->GetID());
                        }
                    }
                }

                for(auto client : clients)
                {
                    zoneManager->MoveToInstance(client, instAccess);

                    if(ctx.Client == client &&
                        client->GetClientState()->GetChannelLogin())
                    {
                        ctx.ChannelChanged = true;
                    }
                }
            }

            return resultCode != 0;
        }
    case objects::ActionZoneInstance::Mode_t::JOIN:
        if(!ctx.Client)
        {
            LOG_ERROR("Attempted to join an instance with no client"
                " context\n");
            return false;
        }
        else
        {
            auto instAccess = zoneManager->GetInstanceAccess(ctx.Client
                ->GetClientState()->GetWorldCID());
            bool success = instAccess && zoneManager->MoveToInstance(
                ctx.Client, instAccess);

            ctx.ChannelChanged = ctx.Client->GetClientState()
                ->GetChannelLogin() != nullptr;

            return success;
        }
        break;
    case objects::ActionZoneInstance::Mode_t::START_TIMER:
        {
            auto instance = ctx.CurrentZone->GetInstance();
            auto def = instance ? instance->GetDefinition() : nullptr;

            if(!instance || (act->GetInstanceID() &&
                def->GetID() != act->GetInstanceID()))
            {
                return false;
            }

            uint32_t timerID = act->GetTimerID();
            if(timerID)
            {
                switch(ctx.CurrentZone->GetInstanceType())
                {
                case InstanceType_t::TIME_TRIAL:
                case InstanceType_t::DEMON_ONLY:
                    LOG_ERROR("Attempted to start a non-default timer on"
                        " an implicit timer instance type.\n");
                    return false;
                default:
                    break;
                }

                // Stop any existing timer
                if(instance->GetTimerStart() && !instance->GetTimerStop() &&
                    !zoneManager->StopInstanceTimer(instance))
                {
                    LOG_ERROR(libcomp::String("Attempted to start an instance"
                        " timer but the previous timer could not be stopped"
                        " first for instance %1\n").Arg(instance->GetID()));
                    return false;
                }

                auto definitionManager = server->GetDefinitionManager();
                auto timeLimitDef = definitionManager->GetTimeLimitData(
                    timerID);
                if(timeLimitDef)
                {
                    instance->SetTimerID(timerID);
                    instance->SetTimeLimitData(timeLimitDef);
                    instance->SetTimerExpirationEventID(
                        act->GetTimerExpirationEventID());
                    instance->SetTimerStart(0);
                    instance->SetTimerStop(0);
                    instance->SetTimerExpire(0);
                }
                else
                {
                    LOG_ERROR(libcomp::String("Attempted to start an invalid"
                        " instance timer: %1\n").Arg(timerID));
                    return false;
                }
            }

            return zoneManager->StartInstanceTimer(instance);
        }
        break;
    case objects::ActionZoneInstance::Mode_t::STOP_TIMER:
        {
            auto instance = ctx.CurrentZone->GetInstance();
            auto def = instance ? instance->GetDefinition() : nullptr;

            if(!instance || (act->GetInstanceID() &&
                def->GetID() != act->GetInstanceID()))
            {
                return false;
            }

            uint32_t timerID = act->GetTimerID();
            if(timerID)
            {
                auto timeLimitData = instance->GetTimeLimitData();
                if(!timeLimitData || timeLimitData->GetID() != timerID)
                {
                    LOG_ERROR(libcomp::String("Attempted to stop an"
                        " instance timer that did not match the supplied"
                        " timer ID: %1\n").Arg(timerID));
                    return false;
                }
            }

            return zoneManager->StopInstanceTimer(instance);
        }
        break;
    case objects::ActionZoneInstance::Mode_t::TEAM_PVP:
        if(!ctx.Client)
        {
            LOG_ERROR("Attempted to start a team PvP match with no client"
                " context\n");
            return false;
        }
        else
        {
            auto matchManager = server->GetMatchManager();
            return matchManager->RequestTeamPvPMatch(ctx.Client,
                act->GetVariantID(), act->GetInstanceID());
        }
        break;
    default:
        break;
    }

    return false;
}

bool ActionManager::Spawn(ActionContext& ctx)
{
    auto act = GetAction<objects::ActionSpawn>(ctx, false);
    if(!act)
    {
        return false;
    }

    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();

    bool spawned = zoneManager->UpdateSpawnGroups(ctx.CurrentZone, true, 0, act);
    switch(act->GetMode())
    {
    case objects::ActionSpawn::Mode_t::ONE_TIME:
    case objects::ActionSpawn::Mode_t::ONE_TIME_RANDOM:
        // Only quit if nothing spawned and it isn't an attempt to spawn
        // to a specific spot ID that has already spawned enemies
        return spawned || ctx.CurrentZone->SpawnedAtSpot(act->GetSpotID());
    case objects::ActionSpawn::Mode_t::DESPAWN:
        // Never quit
        return true;
    default:
        return spawned;
    }
}

bool ActionManager::CreateLoot(ActionContext& ctx)
{
    auto act = GetAction<objects::ActionCreateLoot>(ctx, false);
    if(!act)
    {
        return false;
    }

    auto server = mServer.lock();
    auto characterManager = server->GetCharacterManager();
    auto serverDataManager = server->GetServerDataManager();
    auto zoneManager = server->GetZoneManager();

    auto zone = ctx.CurrentZone;

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

    auto zConnections = zone->GetConnectionList();
    auto firstClient = zConnections.size() > 0 ? zConnections.front()
        : nullptr;

    auto zoneSpots = zone->GetDynamicMap()->Spots;

    uint32_t bossGroupID = ctx.Options.GroupID;
    if(act->GetBossGroupID())
    {
        bossGroupID = act->GetBossGroupID();
    }

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

        // If a spot is specified, get a random point within it
        auto spotIter = zoneSpots.find(loc->GetSpotID());
        if(spotIter != zoneSpots.end())
        {
            Point p = zoneManager->GetRandomSpotPoint(spotIter->second
                ->Definition);
            x = p.x;
            y = p.y;
        }

        lState->SetCurrentX(x);
        lState->SetCurrentY(y);
        lState->SetCurrentRotation(rot);

        lState->SetEntityID(server->GetNextEntityID());
        entityIDs.push_back(lState->GetEntityID());

        zone->AddLootBox(lState, bossGroupID);

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

bool ActionManager::Delay(ActionContext& ctx)
{
    auto act = GetAction<objects::ActionDelay>(ctx, false, false);
    if(!act)
    {
        return false;
    }

    if(act->GetType() == objects::ActionDelay::Type_t::ACTION_DELAY)
    {
        // Execute actions after delay (in seconds)
        if(act->GetDuration())
        {
            uint64_t delayTime = (uint64_t)(ChannelServer::GetServerTime() +
                (uint64_t)((uint64_t)act->GetDuration() * 1000000));

            int32_t worldCID = ctx.Client ? ctx.Client->GetClientState()
                ->GetWorldCID() : 0;

            auto server = mServer.lock();
            server->ScheduleWork(delayTime, [](
                const std::shared_ptr<ChannelServer> pServer,
                const std::shared_ptr<objects::ActionDelay> pAct,
                const std::shared_ptr<Zone> pZone,
                int32_t pSourceEntityID, int32_t pWorldCID,
                uint32_t pGroupID)
                {
                    auto actionManager = pServer->GetActionManager();
                    if(actionManager && !pZone->GetInvalid())
                    {
                        // Only get the client if they're still in the zone
                        auto client = pWorldCID
                            ? pZone->GetConnections()[pWorldCID] : nullptr;

                        ActionOptions options;
                        options.GroupID = pGroupID;

                        actionManager->PerformActions(client,
                            pAct->GetActions(), pSourceEntityID, pZone,
                            options);

                        // Fire action delay triggers
                        if(pAct->GetDelayID() &&
                            pZone->ActionDelayKeysContains(pAct->GetDelayID()))
                        {
                            for(auto trigger : pZone->GetActionDelayTriggers())
                            {
                                if(trigger->GetValue() == pAct->GetDelayID())
                                {
                                    actionManager->PerformActions(client,
                                        trigger->GetActions(), pSourceEntityID,
                                        pZone);
                                }
                            }
                        }
                    }
                }, server, act, ctx.CurrentZone, ctx.SourceEntityID, worldCID,
                    ctx.Options.GroupID);
        }
    }
    else if(act->GetType() == objects::ActionDelay::Type_t::TIMER_EXTEND)
    {
        auto instance = ctx.CurrentZone->GetInstance();
        if(!instance || (act->GetDelayID() > 0 &&
            instance->GetTimerID() != (uint32_t)act->GetDelayID()))
        {
            // No valid timer found
            return false;
        }

        return mServer.lock()->GetZoneManager()->ExtendInstanceTimer(
            instance, (uint32_t)act->GetDuration());
    }
    else if(act->GetDelayID())
    {
        uint32_t sysTime = act->GetDuration()
            ? (uint32_t)((uint32_t)std::time(0) + act->GetDuration())
            : 0;

        auto client = ctx.Client;
        auto state = client ? client->GetClientState() : nullptr;
        switch(act->GetType())
        {
        case objects::ActionDelay::Type_t::CHARACTER_COOLDOWN:
            // Set the context character's ActionCooldowns value
            {
                auto cState = state
                    ? state->GetCharacterState() : nullptr;
                auto character = cState ? cState->GetEntity() : nullptr;
                if(!character)
                {
                    return false;
                }

                if(sysTime)
                {
                    character->SetActionCooldowns(act->GetDelayID(), sysTime);
                }
                else
                {
                    character->RemoveActionCooldowns(act->GetDelayID());
                }

                // If any are invoke cooldowns, send the updated times
                if(act->GetDelayID() == COOLDOWN_INVOKE_LAW ||
                    act->GetDelayID() == COOLDOWN_INVOKE_NEUTRAL ||
                    act->GetDelayID() == COOLDOWN_INVOKE_CHAOS ||
                    act->GetDelayID() == COOLDOWN_INVOKE_WAIT)
                {
                    mServer.lock()->GetCharacterManager()->SendInvokeStatus(
                        ctx.Client, true);
                }
            }
            break;
        case objects::ActionDelay::Type_t::ACCOUNT_COOLDOWN:
            // Set the context account world data's ActionCooldowns value
            {
                auto awd = state ? state->GetAccountWorldData().Get()
                    : nullptr;
                if(!awd)
                {
                    return false;
                }

                if(sysTime)
                {
                    awd->SetActionCooldowns(act->GetDelayID(), sysTime);
                }
                else
                {
                    awd->RemoveActionCooldowns(act->GetDelayID());
                }
            }
            break;
        default:
            break;
        }
    }

    return true;
}

bool ActionManager::RunScript(ActionContext& ctx)
{
    auto act = GetAction<objects::ActionRunScript>(ctx, false, false);
    if(!act)
    {
        return false;
    }

    auto server = mServer.lock();
    auto serverDataManager = server->GetServerDataManager();

    auto script = serverDataManager->GetScript(act->GetScriptID());
    if(script && script->Type.ToLower() == "actioncustom")
    {
        auto engine = std::make_shared<libcomp::ScriptEngine>();

        // Bind some defaults
        engine->Using<ChannelServer>();
        engine->Using<CharacterState>();
        engine->Using<DemonState>();
        engine->Using<EnemyState>();
        engine->Using<Zone>();
        engine->Using<libcomp::Randomizer>();

        // Bind the results enum
        {
            Sqrat::Enumeration e(engine->GetVM());
            e.Const("SUCCESS", (int32_t)ActionRunScriptResult_t::SUCCESS);
            e.Const("FAIL", (int32_t)ActionRunScriptResult_t::FAIL);
            e.Const("LOG_OFF", (int32_t)ActionRunScriptResult_t::LOG_OFF);

            Sqrat::ConstTable(engine->GetVM()).Enum("Result_t", e);
        }

        if(!engine->Eval(script->Source))
        {
            return false;
        }

        Sqrat::Array sqParams(engine->GetVM());
        for(libcomp::String p : act->GetParams())
        {
            sqParams.Append(p);
        }

        int32_t sourceEntityID = ctx.SourceEntityID;
        auto zone = ctx.CurrentZone;
        auto source = zone ? zone->GetActiveEntity(sourceEntityID) : nullptr;

        auto client = ctx.Client;
        auto state = client ? client->GetClientState() : nullptr;
        if(!state)
        {
            state = ClientState::GetEntityClientState(sourceEntityID);
        }

        Sqrat::Function f(Sqrat::RootTable(engine->GetVM()), "run");
        auto scriptResult = !f.IsNull()
            ? f.Evaluate<int32_t>(
                source,
                state ? state->GetCharacterState() : nullptr,
                state ? state->GetDemonState() : nullptr,
                zone,
                server,
                sqParams) : 0;

        if(scriptResult)
        {
            switch((ActionRunScriptResult_t)*scriptResult)
            {
            case ActionRunScriptResult_t::SUCCESS:
                return true;
            case ActionRunScriptResult_t::LOG_OFF:
                {
                    server->GetAccountManager()->RequestDisconnect(client);

                    // Close in case the client ignores it
                    client->Close();

                    return true;
                }
            default:
                // Failure
                break;
            }
        }

        return false;
    }

    return true;
}

bool ActionManager::RecordTimeTrial(ActionContext& ctx, uint32_t rewardItem,
    uint16_t rewardItemCount)
{
    // Push the pending time trial values to the records
    auto state = ctx.Client->GetClientState();
    auto cState = state ? state->GetCharacterState() : nullptr;
    auto character = cState ? cState->GetEntity() : nullptr;
    auto progress = character ? character->GetProgress().Get()
        : nullptr;
    if(progress && progress->GetTimeTrialID())
    {
        int8_t trialID = progress->GetTimeTrialID();

        bool newRecord = false;

        // If the new time was faster, store it in the records
        uint16_t previousTime = progress->GetTimeTrialRecords(
            (size_t)(trialID - 1));
        if(!previousTime || previousTime > progress->GetTimeTrialTime())
        {
            progress->SetTimeTrialRecords((size_t)(trialID - 1),
                progress->GetTimeTrialTime());
            newRecord = true;
        }

        libcomp::Packet p;
        p.WritePacketCode(
            ChannelToClientPacketCode_t::PACKET_TIME_TRIAL_REPORT);
        p.WriteU8(newRecord ? 1 : 0);
        p.WriteS8(trialID);
        p.WriteU16Little(progress->GetTimeTrialTime());
        p.WriteU32Little(rewardItem);
        p.WriteU16Little(rewardItemCount);

        ctx.Client->SendPacket(p);

        progress->SetTimeTrialID(-1);
        progress->SetTimeTrialTime(0);
        progress->SetTimeTrialResult(
            objects::CharacterProgress::TimeTrialResult_t::NONE);

        mServer.lock()->GetWorldDatabase()->QueueUpdate(progress,
            state->GetAccountUID());
    }
    else if(ctx.Action->GetStopOnFailure())
    {
        LOG_ERROR(libcomp::String("Attempted to update an active time"
            " trial record but one does not exist: %1\n")
            .Arg(state->GetAccountUID().ToString()));
        return false;
    }

    return true;
}

bool ActionManager::VerifyClient(ActionContext& ctx,
    const libcomp::String& typeName)
{
    if(!ctx.Client)
    {
        LOG_ERROR(libcomp::String("Attempted to execute a %1 with no"
            " associated client connection\n").Arg(typeName));
        return false;
    }

    return true;
}

bool ActionManager::VerifyZone(ActionContext& ctx,
    const libcomp::String& typeName)
{
    if(!ctx.CurrentZone)
    {
        LOG_ERROR(libcomp::String("Attempted to execute a %1 with no"
            " current zone\n").Arg(typeName));
        return false;
    }

    return true;
}

bool ActionManager::PrepareTransformScript(ActionContext& ctx,
    std::shared_ptr<libcomp::ScriptEngine> engine)
{
    auto serverDataManager = mServer.lock()->GetServerDataManager();
    auto act = ctx.Action;
    auto script = act
        ? serverDataManager->GetScript(act->GetTransformScriptID()) : nullptr;
    if(script && script->Type.ToLower() == "actiontransform")
    {
        // Bind some defaults
        engine->Using<CharacterState>();
        engine->Using<DemonState>();
        engine->Using<EnemyState>();
        engine->Using<Zone>();
        engine->Using<libcomp::Randomizer>();

        auto src = libcomp::String("local action;\n"
            "function prepare(a) { action = a; return 0; }\n%1")
            .Arg(script->Source);
        if(engine->Eval(src))
        {
            return true;
        }
    }

    return false;
}

bool ActionManager::TransformAction(ActionContext& ctx,
    std::shared_ptr<libcomp::ScriptEngine> engine)
{
    auto act = ctx.Action;

    Sqrat::Array sqParams(engine->GetVM());
    for(libcomp::String p : act->GetTransformScriptParams())
    {
        sqParams.Append(p);
    }

    int32_t sourceEntityID = ctx.SourceEntityID;
    auto zone = ctx.CurrentZone;
    auto source = zone ? zone->GetActiveEntity(sourceEntityID) : nullptr;

    auto client = ctx.Client;
    auto state = client ? client->GetClientState() : nullptr;
    if(!state)
    {
        state = ClientState::GetEntityClientState(sourceEntityID);
    }

    Sqrat::Function f(Sqrat::RootTable(engine->GetVM()), "transform");
    auto scriptResult = !f.IsNull()
        ? f.Evaluate<int32_t>(
            source,
            state ? state->GetCharacterState() : nullptr,
            state ? state->GetDemonState() : nullptr,
            zone,
            sqParams) : 0;
    return scriptResult && *scriptResult == 0;
}
