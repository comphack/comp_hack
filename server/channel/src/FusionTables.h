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

#endif // SERVER_CHANNEL_SRC_FUSIONTABLES_H
