/**
 * @file libhack/src/PersistentObjectInitialize.cpp
 * @ingroup libhack
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Initialization of PersistentObject derived types.
 *
 * This file is part of the COMP_hack Library (libhack).
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

#include "PersistentObjectInitialize.h"

// All libhack PersistentObject Includes
#include "Account.h"
#include "AccountWorldData.h"
#include "BazaarData.h"
#include "BazaarItem.h"
#include "Character.h"
#include "CharacterProgress.h"
#include "Clan.h"
#include "ClanMember.h"
#include "CultureData.h"
#include "Demon.h"
#include "DemonBox.h"
#include "DemonQuest.h"
#include "EntityStats.h"
#include "EventCounter.h"
#include "Expertise.h"
#include "FriendSettings.h"
#include "Hotbar.h"
#include "InheritedSkill.h"
#include "Item.h"
#include "ItemBox.h"
#include "PentalphaEntry.h"
#include "PentalphaMatch.h"
#include "PostItem.h"
#include "Promo.h"
#include "PromoExchange.h"
#include "PvPData.h"
#include "Quest.h"
#include "RegisteredChannel.h"
#include "RegisteredWorld.h"
#include "ReportedPlayer.h"
#include "StatusEffect.h"
#include "UBResult.h"
#include "UBTournament.h"

bool libhack::PersistentObjectInitialize() {
  libcomp::PersistentObject::RegisterType(
      typeid(objects::Account), objects::Account::GetMetadata(),
      []() { return (libcomp::PersistentObject*)new objects::Account(); });

  libcomp::PersistentObject::RegisterType(
      typeid(objects::AccountWorldData),
      objects::AccountWorldData::GetMetadata(), []() {
        return (libcomp::PersistentObject*)new objects::AccountWorldData();
      });

  libcomp::PersistentObject::RegisterType(
      typeid(objects::BazaarData), objects::BazaarData::GetMetadata(),
      []() { return (libcomp::PersistentObject*)new objects::BazaarData(); });

  libcomp::PersistentObject::RegisterType(
      typeid(objects::BazaarItem), objects::BazaarItem::GetMetadata(),
      []() { return (libcomp::PersistentObject*)new objects::BazaarItem(); });

  libcomp::PersistentObject::RegisterType(
      typeid(objects::Character), objects::Character::GetMetadata(),
      []() { return (libcomp::PersistentObject*)new objects::Character(); });

  libcomp::PersistentObject::RegisterType(
      typeid(objects::CharacterProgress),
      objects::CharacterProgress::GetMetadata(), []() {
        return (libcomp::PersistentObject*)new objects::CharacterProgress();
      });

  libcomp::PersistentObject::RegisterType(
      typeid(objects::Clan), objects::Clan::GetMetadata(),
      []() { return (libcomp::PersistentObject*)new objects::Clan(); });

  libcomp::PersistentObject::RegisterType(
      typeid(objects::ClanMember), objects::ClanMember::GetMetadata(),
      []() { return (libcomp::PersistentObject*)new objects::ClanMember(); });

  libcomp::PersistentObject::RegisterType(
      typeid(objects::CultureData), objects::CultureData::GetMetadata(),
      []() { return (libcomp::PersistentObject*)new objects::CultureData(); });

  libcomp::PersistentObject::RegisterType(
      typeid(objects::Demon), objects::Demon::GetMetadata(),
      []() { return (libcomp::PersistentObject*)new objects::Demon(); });

  libcomp::PersistentObject::RegisterType(
      typeid(objects::DemonBox), objects::DemonBox::GetMetadata(),
      []() { return (libcomp::PersistentObject*)new objects::DemonBox(); });

  libcomp::PersistentObject::RegisterType(
      typeid(objects::DemonQuest), objects::DemonQuest::GetMetadata(),
      []() { return (libcomp::PersistentObject*)new objects::DemonQuest(); });

  libcomp::PersistentObject::RegisterType(
      typeid(objects::EntityStats), objects::EntityStats::GetMetadata(),
      []() { return (libcomp::PersistentObject*)new objects::EntityStats(); });

  libcomp::PersistentObject::RegisterType(
      typeid(objects::EventCounter), objects::EventCounter::GetMetadata(),
      []() { return (libcomp::PersistentObject*)new objects::EventCounter(); });

  libcomp::PersistentObject::RegisterType(
      typeid(objects::Expertise), objects::Expertise::GetMetadata(),
      []() { return (libcomp::PersistentObject*)new objects::Expertise(); });

  libcomp::PersistentObject::RegisterType(
      typeid(objects::FriendSettings), objects::FriendSettings::GetMetadata(),
      []() {
        return (libcomp::PersistentObject*)new objects::FriendSettings();
      });

  libcomp::PersistentObject::RegisterType(
      typeid(objects::Hotbar), objects::Hotbar::GetMetadata(),
      []() { return (libcomp::PersistentObject*)new objects::Hotbar(); });

  libcomp::PersistentObject::RegisterType(
      typeid(objects::InheritedSkill), objects::InheritedSkill::GetMetadata(),
      []() {
        return (libcomp::PersistentObject*)new objects::InheritedSkill();
      });

  libcomp::PersistentObject::RegisterType(
      typeid(objects::Item), objects::Item::GetMetadata(),
      []() { return (libcomp::PersistentObject*)new objects::Item(); });

  libcomp::PersistentObject::RegisterType(
      typeid(objects::ItemBox), objects::ItemBox::GetMetadata(),
      []() { return (libcomp::PersistentObject*)new objects::ItemBox(); });

  libcomp::PersistentObject::RegisterType(
      typeid(objects::PentalphaEntry), objects::PentalphaEntry::GetMetadata(),
      []() {
        return (libcomp::PersistentObject*)new objects::PentalphaEntry();
      });

  libcomp::PersistentObject::RegisterType(
      typeid(objects::PentalphaMatch), objects::PentalphaMatch::GetMetadata(),
      []() {
        return (libcomp::PersistentObject*)new objects::PentalphaMatch();
      });

  libcomp::PersistentObject::RegisterType(
      typeid(objects::PostItem), objects::PostItem::GetMetadata(),
      []() { return (libcomp::PersistentObject*)new objects::PostItem(); });

  libcomp::PersistentObject::RegisterType(
      typeid(objects::Promo), objects::Promo::GetMetadata(),
      []() { return (libcomp::PersistentObject*)new objects::Promo(); });

  libcomp::PersistentObject::RegisterType(
      typeid(objects::PromoExchange), objects::PromoExchange::GetMetadata(),
      []() {
        return (libcomp::PersistentObject*)new objects::PromoExchange();
      });

  libcomp::PersistentObject::RegisterType(
      typeid(objects::PvPData), objects::PvPData::GetMetadata(),
      []() { return (libcomp::PersistentObject*)new objects::PvPData(); });

  libcomp::PersistentObject::RegisterType(
      typeid(objects::Quest), objects::Quest::GetMetadata(),
      []() { return (libcomp::PersistentObject*)new objects::Quest(); });

  libcomp::PersistentObject::RegisterType(
      typeid(objects::RegisteredChannel),
      objects::RegisteredChannel::GetMetadata(), []() {
        return (libcomp::PersistentObject*)new objects::RegisteredChannel();
      });

  libcomp::PersistentObject::RegisterType(
      typeid(objects::RegisteredWorld), objects::RegisteredWorld::GetMetadata(),
      []() {
        return (libcomp::PersistentObject*)new objects::RegisteredWorld();
      });

  libcomp::PersistentObject::RegisterType(
      typeid(objects::ReportedPlayer), objects::ReportedPlayer::GetMetadata(),
      []() {
        return (libcomp::PersistentObject*)new objects::ReportedPlayer();
      });

  libcomp::PersistentObject::RegisterType(
      typeid(objects::StatusEffect), objects::StatusEffect::GetMetadata(),
      []() { return (libcomp::PersistentObject*)new objects::StatusEffect(); });

  libcomp::PersistentObject::RegisterType(
      typeid(objects::UBResult), objects::UBResult::GetMetadata(),
      []() { return (libcomp::PersistentObject*)new objects::UBResult(); });

  libcomp::PersistentObject::RegisterType(
      typeid(objects::UBTournament), objects::UBTournament::GetMetadata(),
      []() { return (libcomp::PersistentObject*)new objects::UBTournament(); });

  return !libcomp::PersistentObject::InitializationFailed();
}
