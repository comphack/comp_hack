/**
 * @file server/channel/src/FusionTables.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Tables used by fusion operations.
 *
 * This file is part of the channel Server (channel).
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

#include <stdint.h>

#ifndef SERVER_CHANNEL_SRC_FUSIONTABLES_H
#define SERVER_CHANNEL_SRC_FUSIONTABLES_H

// 34x34 2-Way Fusion table of race IDs to resulting race ID or
// elemental ID index (when source IDs match, no enum used)
// The first row represents the positioning of each input race
// on both the row (starting at 1) and column.
extern uint8_t FUSION_RACE_MAP[35][34];

// Set of fusion level up (1), level down (-1) or null (0) fusion
// results when an elemental is only one of the components. The
// row index should match the race index value of FUSION_RACE_MAP
// and the column index used here should match the elemental demon
// index in SVR_CONST (ex: SVR_CONST.ELEMENTAL_1_FLAEMIS).
extern int8_t FUSION_ELEMENTAL_ADJUST[34][4];

// Elemental to elemental fusion table that results in either no
// valid fusion (-1) or a mitama type represented by its corresponding
// mitama index value.
extern int8_t FUSION_ELEMENTAL_MITAMA[4][4];

// Bonus success rates granted by an individual component of two
// way fusion based upon level range (min level in first column)
// and familiarity rank starting at rank +1 (4001 points).
extern uint8_t FUSION_FAMILIARITY_BONUS[5][5];

// Trifusion race priority order to use when determining which
// of two identical base level demons being fused together should
// be considered the "first" one.
extern uint8_t TRIFUSION_RACE_PRIORITY[34];

// Normal trifusion family lookup table to use when determining the
// race outcome of a fusion not overridden by a previous rule. The
// first two indexes of the array should be the top priority demon
// families and the last one should be the lower priority demon.
extern uint8_t TRIFUSION_FAMILY_MAP[7][7][8];

// Inheritence type to skill type lookup table to use when determining
// the base level of inheritence of each skill transferred to a fusion
// result demon. Columns correspond to their respecitve inheritence
// type values. Each row corresponds to their respective affinity
// value - 1.
extern uint8_t INHERITENCE_SKILL_MAP[21][21];

// Point values assigned to each normal reunion rank 0-9 used for
// point extraction and injection.
extern uint16_t REUNION_RANK_POINTS[10];

#endif // SERVER_CHANNEL_SRC_FUSIONTABLES_H
