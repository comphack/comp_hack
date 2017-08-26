/**
 * @file server/channel/src/Packets.h
 * @ingroup channel
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Classes used to parse client channel packets.
 *
 * This file is part of the channel Server (channel).
 *
 * Copyright (C) 2012-2016 COMP_hack Team <compomega@tutanota.com>
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

#ifndef LIBCOMP_SRC_PACKETS_H
#define LIBCOMP_SRC_PACKETS_H

// libcomp Includes
#include <PacketParser.h>

namespace channel
{

namespace Parsers
{

PACKET_PARSER_DECL(Unsupported);
PACKET_PARSER_DECL(Login);                  // 0x0000
PACKET_PARSER_DECL(Auth);                   // 0x0002
PACKET_PARSER_DECL(SendData);               // 0x0004
PACKET_PARSER_DECL(Logout);                 // 0x0005
PACKET_PARSER_DECL(PopulateZone);           // 0x0019
PACKET_PARSER_DECL(Move);                   // 0x001C
PACKET_PARSER_DECL(Chat);                   // 0x0026
PACKET_PARSER_DECL(Tell);                   // 0x0027
PACKET_PARSER_DECL(ActivateSkill);          // 0x0030
PACKET_PARSER_DECL(ExecuteSkill);           // 0x0031
PACKET_PARSER_DECL(CancelSkill);            // 0x0032
PACKET_PARSER_DECL(AllocateSkillPoint);     // 0x0049
PACKET_PARSER_DECL(ToggleExpertise);        // 0x004F
PACKET_PARSER_DECL(LearnSkill);             // 0x0051
PACKET_PARSER_DECL(UpdateDemonSkill);       // 0x0054
PACKET_PARSER_DECL(KeepAlive);              // 0x0056
PACKET_PARSER_DECL(FixObjectPosition);      // 0x0058
PACKET_PARSER_DECL(State);                  // 0x005A
PACKET_PARSER_DECL(PartnerDemonData);       // 0x005B
PACKET_PARSER_DECL(DemonBox);               // 0x005C
PACKET_PARSER_DECL(DemonBoxData);           // 0x005E
PACKET_PARSER_DECL(ChannelList);            // 0x0063
PACKET_PARSER_DECL(ReviveCharacter);        // 0x0067
PACKET_PARSER_DECL(StopMovement);           // 0x006F
PACKET_PARSER_DECL(SpotTriggered);          // 0x0071
PACKET_PARSER_DECL(WorldTime);              // 0x0072
PACKET_PARSER_DECL(ItemBox);                // 0x0074
PACKET_PARSER_DECL(ItemMove);               // 0x0076
PACKET_PARSER_DECL(ItemDrop);               // 0x0077
PACKET_PARSER_DECL(ItemStack);              // 0x0078
PACKET_PARSER_DECL(EquipmentList);          // 0x007B
PACKET_PARSER_DECL(TradeRequest);           // 0x007D
PACKET_PARSER_DECL(TradeAccept);            // 0x0080
PACKET_PARSER_DECL(TradeAddItem);           // 0x0082
PACKET_PARSER_DECL(TradeLock);              // 0x0085
PACKET_PARSER_DECL(TradeFinish);            // 0x0088
PACKET_PARSER_DECL(TradeCancel);            // 0x008B
PACKET_PARSER_DECL(LootItem);               // 0x008E
PACKET_PARSER_DECL(CashBalance);            // 0x0090
PACKET_PARSER_DECL(ShopData);               // 0x0092
PACKET_PARSER_DECL(ShopBuy);                // 0x0094
PACKET_PARSER_DECL(ShopSell);               // 0x0096
PACKET_PARSER_DECL(DemonBoxMove);           // 0x009A
PACKET_PARSER_DECL(DismissDemon);           // 0x009B
PACKET_PARSER_DECL(PostList);               // 0x009C
PACKET_PARSER_DECL(PostItem);               // 0x009E
PACKET_PARSER_DECL(HotbarData);             // 0x00A2
PACKET_PARSER_DECL(HotbarSave);             // 0x00A4
PACKET_PARSER_DECL(EventResponse);          // 0x00B7
PACKET_PARSER_DECL(ValuableList);           // 0x00B8
PACKET_PARSER_DECL(ObjectInteraction);      // 0x00BA
PACKET_PARSER_DECL(FriendInfo);             // 0x00BD
PACKET_PARSER_DECL(FriendRequest);          // 0x00C0
PACKET_PARSER_DECL(FriendAddRemove);        // 0x00C3
PACKET_PARSER_DECL(FriendData);             // 0x00C6
PACKET_PARSER_DECL(PartyInvite);            // 0x00D2
PACKET_PARSER_DECL(PartyJoin);              // 0x00D5
PACKET_PARSER_DECL(PartyCancel);            // 0x00D8
PACKET_PARSER_DECL(PartyLeave);             // 0x00DA
PACKET_PARSER_DECL(PartyDisband);           // 0x00DD
PACKET_PARSER_DECL(PartyLeaderUpdate);      // 0x00E0
PACKET_PARSER_DECL(PartyDropRule);          // 0x00E3
PACKET_PARSER_DECL(PartyKick);              // 0x00EB
PACKET_PARSER_DECL(DemonFusion);            // 0x00EF
PACKET_PARSER_DECL(LootDemonEggData);       // 0x00F1
PACKET_PARSER_DECL(Sync);                   // 0x00F3
PACKET_PARSER_DECL(Rotate);                 // 0x00F8
PACKET_PARSER_DECL(LootBossBox);            // 0x00FC
PACKET_PARSER_DECL(UnionFlag);              // 0x0100
PACKET_PARSER_DECL(ItemDepoList);           // 0x0102
PACKET_PARSER_DECL(DepoRent);               // 0x0104
PACKET_PARSER_DECL(LootTreasureBox);        // 0x0107
PACKET_PARSER_DECL(QuestActiveList);        // 0x010B
PACKET_PARSER_DECL(QuestCompletedList);     // 0x010C
PACKET_PARSER_DECL(ClanDisband);            // 0x0138
PACKET_PARSER_DECL(ClanInvite);             // 0x013B
PACKET_PARSER_DECL(ClanJoin);               // 0x013E
PACKET_PARSER_DECL(ClanCancel);             // 0x0140
PACKET_PARSER_DECL(ClanKick);               // 0x0142
PACKET_PARSER_DECL(ClanMasterUpdate);       // 0x0145
PACKET_PARSER_DECL(ClanSubMasterUpdate);    // 0x0148
PACKET_PARSER_DECL(ClanLeave);              // 0x014B
PACKET_PARSER_DECL(ClanChat);               // 0x014E
PACKET_PARSER_DECL(ClanInfo);               // 0x0150
PACKET_PARSER_DECL(ClanList);               // 0x0152
PACKET_PARSER_DECL(ClanData);               // 0x0154
PACKET_PARSER_DECL(ClanForm);               // 0x0156
PACKET_PARSER_DECL(SyncCharacter);          // 0x017E
PACKET_PARSER_DECL(MapFlag);                // 0x0197
PACKET_PARSER_DECL(DemonCompendium);        // 0x019B
PACKET_PARSER_DECL(DungeonRecords);         // 0x01C4
PACKET_PARSER_DECL(ClanEmblemUpdate);       // 0x01E1
PACKET_PARSER_DECL(DemonFamiliarity);       // 0x01E6
PACKET_PARSER_DECL(MaterialBox);            // 0x0205
PACKET_PARSER_DECL(Analyze);                // 0x0209
PACKET_PARSER_DECL(FusionGauge);            // 0x0217
PACKET_PARSER_DECL(TitleList);              // 0x021B
PACKET_PARSER_DECL(PartnerDemonQuestList);  // 0x022D
PACKET_PARSER_DECL(LockDemon);              // 0x0233
PACKET_PARSER_DECL(PvPCharacterInfo);       // 0x024D
PACKET_PARSER_DECL(TeamInfo);               // 0x027B
PACKET_PARSER_DECL(PartnerDemonQuestTemp);  // 0x028F
PACKET_PARSER_DECL(ItemDepoRemote);         // 0x0296
PACKET_PARSER_DECL(DemonDepoRemote);        // 0x02EF
PACKET_PARSER_DECL(CommonSwitchInfo);       // 0x02F4
PACKET_PARSER_DECL(CasinoCoinTotal);        // 0x02FA
PACKET_PARSER_DECL(SearchEntryInfo);        // 0x03A3
PACKET_PARSER_DECL(HouraiData);             // 0x03A5
PACKET_PARSER_DECL(CultureData);            // 0x03AC
PACKET_PARSER_DECL(DemonDepoList);          // 0x03F5
PACKET_PARSER_DECL(Blacklist);              // 0x0408
PACKET_PARSER_DECL(DigitalizePoints);       // 0x0414
PACKET_PARSER_DECL(DigitalizeAssist);       // 0x0418

PACKET_PARSER_DECL(SetWorldInfo);        // 0x1002
PACKET_PARSER_DECL(SetOtherChannelInfo); // 0x1003
PACKET_PARSER_DECL(AccountLogin);        // 0x1004
PACKET_PARSER_DECL(AccountLogout);       // 0x1005
PACKET_PARSER_DECL(Relay);               // 0x1006
PACKET_PARSER_DECL(CharacterLogin);      // 0x1007
PACKET_PARSER_DECL(FriendsUpdate);       // 0x1008
PACKET_PARSER_DECL(PartyUpdate);         // 0x1009
PACKET_PARSER_DECL(ClanUpdate);          // 0x100A

} // namespace Parsers

} // namespace channel

#endif // LIBCOMP_SRC_PACKETS_H
