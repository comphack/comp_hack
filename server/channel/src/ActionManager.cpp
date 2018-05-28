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
#include <ServerDataManager.h>

// Standard C++11 Includes
#include <math.h>

// object Includes
#include <Account.h>
#include <AccountLogin.h>
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
#include <ActionUpdatePoints.h>
#include <ActionUpdateQuest.h>
#include <ActionUpdateZoneFlags.h>
#include <ActionZoneInstance.h>
#include <ActionZoneChange.h>
#include <CharacterProgress.h>
#include <DemonBox.h>
#include <DropSet.h>
#include <LootBox.h>
#include <MiCategoryData.h>
#include <MiItemData.h>
#include <MiPossessionData.h>
#include <MiSkillItemStatusCommonData.h>
#include <MiSpotData.h>
#include <MiTimeLimitData.h>
#include <MiZoneData.h>
#include <ObjectPosition.h>
#include <Party.h>
#include <PostItem.h>
#include <Quest.h>
#include <ServerNPC.h>
#include <ServerObject.h>
#include <ServerZone.h>
#include <ServerZoneInstance.h>
#include <ServerZoneTrigger.h>

// channel Includes
#include "AccountManager.h"
#include "ChannelServer.h"
#include "CharacterManager.h"
#include "EventManager.h"
#include "ManagerConnection.h"
#include "TokuseiManager.h"
#include "ZoneManager.h"

using namespace channel;

struct channel::ActionContext
{
    std::shared_ptr<ChannelClientConnection> Client;
    std::shared_ptr<objects::Action> Action;
    int32_t SourceEntityID = 0;
    uint32_t GroupID = 0;
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
}

ActionManager::~ActionManager()
{
}

void ActionManager::PerformActions(
    const std::shared_ptr<ChannelClientConnection>& client,
    const std::list<std::shared_ptr<objects::Action>>& actions,
    int32_t sourceEntityID, const std::shared_ptr<Zone>& zone,
    uint32_t groupID)
{
    ActionContext ctx;
    ctx.Client = client;
    ctx.SourceEntityID = sourceEntityID;
    ctx.GroupID = groupID;

    if(zone)
    {
        ctx.CurrentZone = zone;
    }
    else if(ctx.Client)
    {
        ctx.CurrentZone = mServer.lock()->GetZoneManager()
            ->GetCurrentZone(ctx.Client);
    }

    if(!ctx.CurrentZone)
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
            continue;
        }
        else
        {
            auto srcCtx = action->GetSourceContext();
            if(srcCtx != objects::Action::SourceContext_t::SOURCE)
            {
                auto connectionManager = mServer.lock()->GetManagerConnection();

                // Execute once per source context character and quit afterwards
                // if any fail
                bool failure = false;
                bool preFiltered = false;

                std::set<int32_t> worldCIDs;
                switch(srcCtx)
                {
                case objects::Action::SourceContext_t::ALL:
                    // Sub-divide by location
                    switch(action->GetLocation())
                    {
                    case objects::Action::Location_t::INSTANCE:
                        {
                            std::list<std::shared_ptr<Zone>> zones;
                            auto instance = ctx.CurrentZone->GetInstance();
                            if(instance)
                            {
                                for(auto zPair1 : instance->GetZones())
                                {
                                    for(auto zPair2 : zPair1.second)
                                    {
                                        zones.push_back(zPair2.second);
                                    }
                                }
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
                        for(auto conn : ctx.CurrentZone->GetConnectionList())
                        {
                            auto state = conn->GetClientState();
                            if(state)
                            {
                                worldCIDs.insert(state->GetWorldCID());
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
                    case objects::Action::Location_t::WORLD:
                    default:
                        // Not supported
                        failure = true;
                        break;
                    }

                    preFiltered = true;
                    break;
                case objects::Action::SourceContext_t::PARTY:
                    {
                        auto sourceClient = client ? client : connectionManager
                            ->GetEntityClient(sourceEntityID, false);
                        if(!sourceClient)
                        {
                            failure = true;
                            break;
                        }

                        auto state = sourceClient->GetClientState();
                        auto party = state->GetParty();
                        if(party)
                        {
                            worldCIDs = party->GetMemberIDs();
                        }

                        // Always include self in party
                        worldCIDs.insert(state->GetWorldCID());
                    }
                    break;
                default:
                    break;
                }

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
                        auto ctxInst = ctxZone->GetInstance();
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
                        case objects::Action::Location_t::WORLD:
                            /// @todo: handle cross channel actions
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

                            failure |= !it->second(*this, copyCtx);
                        }
                    }
                }

                if(failure)
                {
                    break;
                }
            }
            else if(!it->second(*this, ctx) && action->GetStopOnFailure())
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
            else if(!ctx.CurrentZone)
            {
                LOG_DEBUG(libcomp::String("Quitting mid-action execution"
                    " following removal of the context zone from action"
                    " type: %1.\n").Arg((int32_t)action->GetActionType()));
                break;
            }
        }
    }
}

bool ActionManager::StartEvent(ActionContext& ctx)
{
    auto act = std::dynamic_pointer_cast<objects::ActionStartEvent>(ctx.Action);

    auto server = mServer.lock();
    auto eventManager = server->GetEventManager();

    eventManager->HandleEvent(ctx.Client, act->GetEventID(), ctx.SourceEntityID,
        ctx.CurrentZone, ctx.GroupID);
    
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

    auto currentInstance = ctx.CurrentZone ? ctx.CurrentZone->GetInstance() : nullptr;

    uint32_t spotID = act->GetSpotID();
    if(!zoneID && !spotID)
    {
        // Spot 0, zone 0 is a request to go to the homepoint
        auto character = ctx.Client->GetClientState()->GetCharacterState()->GetEntity();
        zoneID = character ? character->GetHomepointZone() : 0;
        spotID = character ? character->GetHomepointSpotID() : 0;

        if(!zoneID)
        {
            LOG_ERROR("Attempted to move to the homepoint but no"
                " homepoint is set\n");
            return false;
        }
    }
    else if(zoneID && !dynamicMapID && currentInstance)
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

    if(spotID > 0)
    {
        // If a spot is specified, get a random point in that spot instead
        std::shared_ptr<objects::ServerZone> zoneDef;

        if(zoneID == 0)
        {
            // Request is actually to move within the zone
            zoneDef = ctx.CurrentZone->GetDefinition();
            zoneID = zoneDef->GetID();
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

    return ctx.CurrentZone != nullptr;
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
        else if(itemPair.second > 0)
        {
            adds[itemPair.first] = (uint32_t)itemPair.second;
        }
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
    default:
        return false;
    }

    return true;
}

bool ActionManager::AddRemoveStatus(ActionContext& ctx)
{
    auto act = std::dynamic_pointer_cast<objects::ActionAddRemoveStatus>(ctx.Action);

    auto state = ctx.Client ? ctx.Client->GetClientState() : nullptr;
    auto server = mServer.lock();
    auto definitionManager = server->GetDefinitionManager();
    auto tokuseiManager = server->GetTokuseiManager();

    StatusEffectChanges effects;
    for(auto pair : act->GetStatusStacks())
    {
        effects[pair.first] = StatusEffectChange(pair.first, pair.second,
            act->GetIsReplace());
    }

    if(effects.size() > 0)
    {
        bool playerEntityModified = false;
        switch(act->GetTargetType())
        {
        case objects::ActionAddRemoveStatus::TargetType_t::CHARACTER:
            if(state)
            {
                state->GetCharacterState()->AddStatusEffects(effects,
                    definitionManager);
                playerEntityModified = true;
            }
            break;
        case objects::ActionAddRemoveStatus::TargetType_t::PARTNER:
            if(state)
            {
                state->GetDemonState()->AddStatusEffects(effects,
                    definitionManager);
                playerEntityModified = true;
            }
            break;
        case objects::ActionAddRemoveStatus::TargetType_t::CHARACTER_AND_PARTNER:
            if(state)
            {
                state->GetCharacterState()->AddStatusEffects(effects,
                    definitionManager);
                state->GetDemonState()->AddStatusEffects(effects,
                    definitionManager);
                playerEntityModified = true;
            }
            break;
        case objects::ActionAddRemoveStatus::TargetType_t::SOURCE:
            {
                auto eState = ctx.CurrentZone->GetActiveEntity(ctx.SourceEntityID);
                if(eState)
                {
                    eState->AddStatusEffects(effects, definitionManager);
                    tokuseiManager->Recalculate(eState, true);
                }
            }
            break;
        }

        if(playerEntityModified)
        {
            // Recalculate the character and demon
            tokuseiManager->Recalculate(state->GetCharacterState(), true);
        }
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

    auto server = mServer.lock();
    auto characterManager = server->GetCharacterManager();
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

        if(act->GetExpertiseMax() > 0)
        {
            auto character = state->GetCharacterState()->GetEntity();
            uint8_t val = act->GetExpertiseMax();

            int8_t newVal = (int8_t)((character->GetExpertiseExtension() + val) > 127
                ? 127 : (character->GetExpertiseExtension() + val));

            if(newVal != character->GetExpertiseExtension())
            {
                character->SetExpertiseExtension(newVal);

                characterManager->SendExertiseExtension(ctx.Client);
                server->GetWorldDatabase()->QueueUpdate(character,
                    state->GetAccountUID());
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

bool ActionManager::UpdatePoints(ActionContext& ctx)
{
    if(!ctx.Client)
    {
        LOG_ERROR("Attempted to execute an update points action with no"
            " associated client connection\n");
        return false;
    }

    auto act = std::dynamic_pointer_cast<objects::ActionUpdatePoints>(ctx.Action);
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
            LOG_ERROR("Attempted to add digitalize points which are not"
                " supported yet!\n");
        }
        break;
    case objects::ActionUpdatePoints::PointType_t::SOUL_POINTS:
        {
            mServer.lock()->GetCharacterManager()->UpdateSoulPoints(
                ctx.Client, (int32_t)act->GetValue(), !act->GetIsSet());
        }
        break;
    default:
        break;
    }

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

    return eventManager->UpdateQuest(ctx.Client, act->GetQuestID(), act->GetPhase(),
        act->GetForceUpdate(), flagStates);
}

bool ActionManager::UpdateZoneFlags(ActionContext& ctx)
{
    auto act = std::dynamic_pointer_cast<objects::ActionUpdateZoneFlags>(ctx.Action);

    // Determine if it affects the current character or the whole zone
    int32_t worldCID = 0;
    switch(act->GetType())
    {
    case objects::ActionUpdateZoneFlags::Type_t::ZONE_CHARACTER:
    case objects::ActionUpdateZoneFlags::Type_t::ZONE_INSTANCE_CHARACTER:
        worldCID = ctx.Client->GetClientState()->GetWorldCID();
        break;
    default:
        break;
    }

    switch (act->GetType())
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

                    val = val + (incr ? 1 : -1);
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

                        val = val + (incr ? 1 : -1);
                        instance->SetFlagState(pair.first, val, worldCID);
                    }
                }
                break;
            default:
                break;
            }
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
                        ctx.SourceEntityID, ctx.CurrentZone, 0);
                }
            }
        }
    }

    return true;
}

bool ActionManager::UpdateZoneInstance(ActionContext& ctx)
{
    if(!ctx.Client)
    {
        LOG_ERROR("Attempted to execute a zone instance update action with no"
            " associated client connection\n");
        return false;
    }

    auto act = std::dynamic_pointer_cast<objects::ActionZoneInstance>(ctx.Action);
    auto server = mServer.lock();
    auto zoneManager = server->GetZoneManager();

    switch(act->GetMode())
    {
    case objects::ActionZoneInstance::Mode_t::CREATE:
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

            return zoneManager->CreateInstance(ctx.Client,
                act->GetInstanceID(), act->GetVariantID(), act->GetTimerID(),
                act->GetTimerExpirationEventID()) != nullptr;
        }
    case objects::ActionZoneInstance::Mode_t::JOIN:
        {
            auto instance = zoneManager->GetInstanceAccess(ctx.Client);
            auto def = instance ? instance->GetDefinition() : nullptr;
            if(instance && (act->GetInstanceID() == 0 ||
                def->GetID() == act->GetInstanceID()))
            {
                uint32_t firstZoneID = *def->ZoneIDsBegin();
                uint32_t firstDynamicMapID = *def->DynamicMapIDsBegin();

                auto zoneDef = server->GetServerDataManager()->GetZoneData(
                    firstZoneID, firstDynamicMapID);
                return zoneManager->EnterZone(ctx.Client, firstZoneID,
                    firstDynamicMapID, zoneDef->GetStartingX(),
                    zoneDef->GetStartingY(), zoneDef->GetStartingRotation());
            }

            return false;
        }
        break;
    case objects::ActionZoneInstance::Mode_t::REMOVE:
        {
            auto state = ctx.Client->GetClientState();
            auto zone = state->GetZone();
            auto currentInst = zone ? zone->GetInstance() : nullptr;

            auto instance = zoneManager->GetInstanceAccess(ctx.Client);
            if(instance && instance == currentInst)
            {
                zoneManager->ClearInstanceAccess(instance->GetID());
            }

            return true;
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
    default:
        break;
    }

    return false;
}

bool ActionManager::Spawn(ActionContext& ctx)
{
    auto act = std::dynamic_pointer_cast<objects::ActionSpawn>(ctx.Action);
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
    case objects::ActionSpawn::Mode_t::ENABLE_GROUP:
    case objects::ActionSpawn::Mode_t::DISABLE_GROUP:
        // Never quit
        return true;
    default:
        return spawned;
    }
}

bool ActionManager::CreateLoot(ActionContext& ctx)
{
    auto act = std::dynamic_pointer_cast<objects::ActionCreateLoot>(ctx.Action);

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

        zone->AddLootBox(lState, ctx.GroupID);

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
