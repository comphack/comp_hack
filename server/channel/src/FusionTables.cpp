/**
* @file server/channel/src/FusionTables.cpp
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
 
#include "FusionTables.h"

// object includes
#include <MiDCategoryData.h>

typedef objects::MiDCategoryData::Race_t RaceID;

uint8_t FUSION_RACE_MAP[35][34] =
{
// Idx Map
{
(uint8_t)RaceID::SERAPHIM,
(uint8_t)RaceID::ENTITY,
(uint8_t)RaceID::DEMON_GOD,
(uint8_t)RaceID::VILE,
(uint8_t)RaceID::AVIAN,
(uint8_t)RaceID::GODDESS,
(uint8_t)RaceID::HEAVENLY_GOD,
(uint8_t)RaceID::RAPTOR,
(uint8_t)RaceID::DIVINE,
(uint8_t)RaceID::EVIL_DEMON,
(uint8_t)RaceID::WILD_BIRD,
(uint8_t)RaceID::YOMA,
(uint8_t)RaceID::EARTH_ELEMENT,
(uint8_t)RaceID::REAPER,
(uint8_t)RaceID::HOLY_BEAST,
(uint8_t)RaceID::BEAST,
(uint8_t)RaceID::FAIRY,
(uint8_t)RaceID::DEMIGOD,
(uint8_t)RaceID::WILDER,
(uint8_t)RaceID::DRAGON_KING,
(uint8_t)RaceID::NOCTURNE,
(uint8_t)RaceID::GODLY_BEAST,
(uint8_t)RaceID::FOUL,
(uint8_t)RaceID::BRUTE,
(uint8_t)RaceID::HAUNT,
(uint8_t)RaceID::DRAGON,
(uint8_t)RaceID::FALLEN_ANGEL,
(uint8_t)RaceID::FEMME,
(uint8_t)RaceID::NATION_RULER,
(uint8_t)RaceID::EARTH_MOTHER,
(uint8_t)RaceID::EVIL_DRAGON,
(uint8_t)RaceID::GUARDIAN,
(uint8_t)RaceID::DESTROYER,
(uint8_t)RaceID::TYRANT
},
// 大天使: SERAPHIM
{
0,                              // x SERAPHIM
(uint8_t)RaceID::GODDESS,       // x ENTITY
0,                              // x DEMON_GOD
(uint8_t)RaceID::DIVINE,        // x VILE
(uint8_t)RaceID::DIVINE,        // x AVIAN
(uint8_t)RaceID::EARTH_MOTHER,  // x GODDESS
(uint8_t)RaceID::NATION_RULER,  // x HEAVENLY_GOD
(uint8_t)RaceID::REAPER,        // x RAPTOR
(uint8_t)RaceID::GODDESS,       // x DIVINE
0,                              // x EVIL_DEMON
(uint8_t)RaceID::AVIAN,         // x WILD_BIRD
(uint8_t)RaceID::GODDESS,       // x YOMA
(uint8_t)RaceID::HEAVENLY_GOD,  // x EARTH_ELEMENT
0,                              // x REAPER
(uint8_t)RaceID::GODLY_BEAST,   // x HOLY_BEAST
0,                              // x BEAST
(uint8_t)RaceID::HOLY_BEAST,    // x FAIRY
(uint8_t)RaceID::GODDESS,       // x DEMIGOD
0,                              // x WILDER
0,                              // x DRAGON_KING
(uint8_t)RaceID::FALLEN_ANGEL,  // x NOCTURNE
(uint8_t)RaceID::AVIAN,         // x GODLY_BEAST
(uint8_t)RaceID::FALLEN_ANGEL,  // x FOUL
0,                              // x BRUTE
(uint8_t)RaceID::FALLEN_ANGEL,  // x HAUNT
(uint8_t)RaceID::HOLY_BEAST,    // x DRAGON
(uint8_t)RaceID::EARTH_MOTHER,  // x FALLEN_ANGEL
0,                              // x FEMME
(uint8_t)RaceID::DRAGON,        // x NATION_RULER
(uint8_t)RaceID::HEAVENLY_GOD,  // x EARTH_MOTHER
0,                              // x EVIL_DRAGON
(uint8_t)RaceID::NATION_RULER,  // x GUARDIAN
(uint8_t)RaceID::VILE,          // x DESTROYER
(uint8_t)RaceID::FALLEN_ANGEL   // x TYRANT
},
// 威霊: ENTITY
{
(uint8_t)RaceID::GODDESS,       // x SERAPHIM
0,                              // x ENTITY
(uint8_t)RaceID::GODDESS,       // x DEMON_GOD
(uint8_t)RaceID::REAPER,        // x VILE
(uint8_t)RaceID::HEAVENLY_GOD,  // x AVIAN
(uint8_t)RaceID::EARTH_MOTHER,  // x GODDESS
(uint8_t)RaceID::GUARDIAN,      // x HEAVENLY_GOD
(uint8_t)RaceID::VILE,          // x RAPTOR
(uint8_t)RaceID::GODDESS,       // x DIVINE
(uint8_t)RaceID::VILE,          // x EVIL_DEMON
(uint8_t)RaceID::AVIAN,         // x WILD_BIRD
(uint8_t)RaceID::GODDESS,       // x YOMA
(uint8_t)RaceID::NATION_RULER,  // x EARTH_ELEMENT
(uint8_t)RaceID::VILE,          // x REAPER
(uint8_t)RaceID::GUARDIAN,      // x HOLY_BEAST
(uint8_t)RaceID::HOLY_BEAST,    // x BEAST
(uint8_t)RaceID::DIVINE,        // x FAIRY
(uint8_t)RaceID::GUARDIAN,      // x DEMIGOD
(uint8_t)RaceID::BRUTE,         // x WILDER
(uint8_t)RaceID::HEAVENLY_GOD,  // x DRAGON_KING
(uint8_t)RaceID::BRUTE,         // x NOCTURNE
(uint8_t)RaceID::HEAVENLY_GOD,  // x GODLY_BEAST
0,                              // x FOUL
(uint8_t)RaceID::NATION_RULER,  // x BRUTE
0,                              // x HAUNT
(uint8_t)RaceID::EARTH_MOTHER,  // x DRAGON
(uint8_t)RaceID::GUARDIAN,      // x FALLEN_ANGEL
(uint8_t)RaceID::EARTH_MOTHER,  // x FEMME
(uint8_t)RaceID::DRAGON,        // x NATION_RULER
(uint8_t)RaceID::HEAVENLY_GOD,  // x EARTH_MOTHER
0,                              // x EVIL_DRAGON
(uint8_t)RaceID::GODDESS,       // x GUARDIAN
(uint8_t)RaceID::EARTH_MOTHER,  // x DESTROYER
(uint8_t)RaceID::VILE           // x TYRANT
},
// 魔神: DEMON_GOD
{
0,                              // x SERAPHIM
(uint8_t)RaceID::GODDESS,       // x ENTITY
0,                              // x DEMON_GOD
(uint8_t)RaceID::GUARDIAN,      // x VILE
(uint8_t)RaceID::HEAVENLY_GOD,  // x AVIAN
(uint8_t)RaceID::HEAVENLY_GOD,  // x GODDESS
(uint8_t)RaceID::NATION_RULER,  // x HEAVENLY_GOD
(uint8_t)RaceID::VILE,          // x RAPTOR
(uint8_t)RaceID::GODDESS,       // x DIVINE
0,                              // x EVIL_DEMON
(uint8_t)RaceID::FALLEN_ANGEL,  // x WILD_BIRD
(uint8_t)RaceID::DEMIGOD,       // x YOMA
(uint8_t)RaceID::BRUTE,         // x EARTH_ELEMENT
0,                              // x REAPER
(uint8_t)RaceID::AVIAN,         // x HOLY_BEAST
(uint8_t)RaceID::GODLY_BEAST,   // x BEAST
(uint8_t)RaceID::NOCTURNE,      // x FAIRY
0,                              // x DEMIGOD
(uint8_t)RaceID::BEAST,         // x WILDER
(uint8_t)RaceID::GUARDIAN,      // x DRAGON_KING
(uint8_t)RaceID::VILE,          // x NOCTURNE
(uint8_t)RaceID::DRAGON,        // x GODLY_BEAST
0,                              // x FOUL
(uint8_t)RaceID::GUARDIAN,      // x BRUTE
0,                              // x HAUNT
(uint8_t)RaceID::NATION_RULER,  // x DRAGON
0,                              // x FALLEN_ANGEL
(uint8_t)RaceID::EARTH_MOTHER,  // x FEMME
(uint8_t)RaceID::HEAVENLY_GOD,  // x NATION_RULER
0,                              // x EARTH_MOTHER
0,                              // x EVIL_DRAGON
(uint8_t)RaceID::HEAVENLY_GOD,  // x GUARDIAN
0,                              // x DESTROYER
0                               // x TYRANT
},
// 邪神: VILE
{
(uint8_t)RaceID::DIVINE,        // x SERAPHIM
(uint8_t)RaceID::REAPER,        // x ENTITY
(uint8_t)RaceID::GUARDIAN,      // x DEMON_GOD
0,                              // x VILE
(uint8_t)RaceID::REAPER,        // x AVIAN
(uint8_t)RaceID::REAPER,        // x GODDESS
0,                              // x HEAVENLY_GOD
(uint8_t)RaceID::EVIL_DRAGON,   // x RAPTOR
(uint8_t)RaceID::REAPER,        // x DIVINE
(uint8_t)RaceID::FOUL,          // x EVIL_DEMON
(uint8_t)RaceID::EVIL_DRAGON,   // x WILD_BIRD
(uint8_t)RaceID::EARTH_ELEMENT, // x YOMA
(uint8_t)RaceID::HAUNT,         // x EARTH_ELEMENT
(uint8_t)RaceID::FOUL,          // x REAPER
0,                              // x HOLY_BEAST
(uint8_t)RaceID::FOUL,          // x BEAST
(uint8_t)RaceID::RAPTOR,        // x FAIRY
(uint8_t)RaceID::NATION_RULER,  // x DEMIGOD
(uint8_t)RaceID::FOUL,          // x WILDER
(uint8_t)RaceID::GUARDIAN,      // x DRAGON_KING
(uint8_t)RaceID::HAUNT,         // x NOCTURNE
(uint8_t)RaceID::EVIL_DRAGON,   // x GODLY_BEAST
(uint8_t)RaceID::HAUNT,         // x FOUL
(uint8_t)RaceID::FOUL,          // x BRUTE
(uint8_t)RaceID::FOUL,          // x HAUNT
(uint8_t)RaceID::DRAGON_KING,   // x DRAGON
(uint8_t)RaceID::BRUTE,         // x FALLEN_ANGEL
(uint8_t)RaceID::BRUTE,         // x FEMME
(uint8_t)RaceID::REAPER,        // x NATION_RULER
(uint8_t)RaceID::REAPER,        // x EARTH_MOTHER
(uint8_t)RaceID::WILDER,        // x EVIL_DRAGON
(uint8_t)RaceID::REAPER,        // x GUARDIAN
(uint8_t)RaceID::REAPER,        // x DESTROYER
(uint8_t)RaceID::REAPER         // x TYRANT
},
// 霊鳥: AVIAN
{
(uint8_t)RaceID::DIVINE,        // x SERAPHIM
(uint8_t)RaceID::HEAVENLY_GOD,  // x ENTITY
(uint8_t)RaceID::HEAVENLY_GOD,  // x DEMON_GOD
(uint8_t)RaceID::REAPER,        // x VILE
3,                              // x AVIAN
(uint8_t)RaceID::HEAVENLY_GOD,  // x GODDESS
(uint8_t)RaceID::DRAGON,        // x HEAVENLY_GOD
0,                              // x RAPTOR
(uint8_t)RaceID::DRAGON_KING,   // x DIVINE
(uint8_t)RaceID::RAPTOR,        // x EVIL_DEMON
(uint8_t)RaceID::RAPTOR,        // x WILD_BIRD
(uint8_t)RaceID::NOCTURNE,      // x YOMA
(uint8_t)RaceID::DIVINE,        // x EARTH_ELEMENT
(uint8_t)RaceID::RAPTOR,        // x REAPER
(uint8_t)RaceID::EARTH_MOTHER,  // x HOLY_BEAST
(uint8_t)RaceID::DRAGON_KING,   // x BEAST
(uint8_t)RaceID::WILD_BIRD,     // x FAIRY
(uint8_t)RaceID::HEAVENLY_GOD,  // x DEMIGOD
(uint8_t)RaceID::WILD_BIRD,     // x WILDER
(uint8_t)RaceID::HEAVENLY_GOD,  // x DRAGON_KING
(uint8_t)RaceID::FEMME,         // x NOCTURNE
(uint8_t)RaceID::HOLY_BEAST,    // x GODLY_BEAST
0,                              // x FOUL
(uint8_t)RaceID::DEMIGOD,       // x BRUTE
0,                              // x HAUNT
(uint8_t)RaceID::GODLY_BEAST,   // x DRAGON
(uint8_t)RaceID::DRAGON_KING,   // x FALLEN_ANGEL
(uint8_t)RaceID::GODDESS,       // x FEMME
(uint8_t)RaceID::HEAVENLY_GOD,  // x NATION_RULER
(uint8_t)RaceID::DRAGON,        // x EARTH_MOTHER
0,                              // x EVIL_DRAGON
(uint8_t)RaceID::DRAGON,        // x GUARDIAN
(uint8_t)RaceID::GUARDIAN,      // x DESTROYER
0                               // x TYRANT
},
// 女神: GODDESS
{
(uint8_t)RaceID::EARTH_MOTHER,  // x SERAPHIM
(uint8_t)RaceID::EARTH_MOTHER,  // x ENTITY
(uint8_t)RaceID::HEAVENLY_GOD,  // x DEMON_GOD
(uint8_t)RaceID::REAPER,        // x VILE
(uint8_t)RaceID::HEAVENLY_GOD,  // x AVIAN
0,                              // x GODDESS
0,                              // x HEAVENLY_GOD
(uint8_t)RaceID::FALLEN_ANGEL,  // x RAPTOR
(uint8_t)RaceID::HEAVENLY_GOD,  // x DIVINE
(uint8_t)RaceID::FEMME,         // x EVIL_DEMON
(uint8_t)RaceID::AVIAN,         // x WILD_BIRD
(uint8_t)RaceID::FEMME,         // x YOMA
(uint8_t)RaceID::EARTH_MOTHER,  // x EARTH_ELEMENT
(uint8_t)RaceID::VILE,          // x REAPER
(uint8_t)RaceID::GODLY_BEAST,   // x HOLY_BEAST
(uint8_t)RaceID::HOLY_BEAST,    // x BEAST
(uint8_t)RaceID::DEMIGOD,       // x FAIRY
(uint8_t)RaceID::HEAVENLY_GOD,  // x DEMIGOD
(uint8_t)RaceID::BEAST,         // x WILDER
(uint8_t)RaceID::FAIRY,         // x DRAGON_KING
(uint8_t)RaceID::FALLEN_ANGEL,  // x NOCTURNE
(uint8_t)RaceID::BEAST,         // x GODLY_BEAST
0,                              // x FOUL
(uint8_t)RaceID::FEMME,         // x BRUTE
(uint8_t)RaceID::VILE,          // x HAUNT
(uint8_t)RaceID::GODLY_BEAST,   // x DRAGON
(uint8_t)RaceID::DIVINE,        // x FALLEN_ANGEL
(uint8_t)RaceID::EARTH_MOTHER,  // x FEMME
(uint8_t)RaceID::EARTH_MOTHER,  // x NATION_RULER
0,                              // x EARTH_MOTHER
(uint8_t)RaceID::VILE,          // x EVIL_DRAGON
(uint8_t)RaceID::EARTH_MOTHER,  // x GUARDIAN
(uint8_t)RaceID::FEMME,         // x DESTROYER
0                               // x TYRANT
},
// 天津神: HEAVENLY_GOD
{
(uint8_t)RaceID::NATION_RULER,  // x SERAPHIM
(uint8_t)RaceID::GUARDIAN,      // x ENTITY
(uint8_t)RaceID::NATION_RULER,  // x DEMON_GOD
0,                              // x VILE
(uint8_t)RaceID::DRAGON,        // x AVIAN
0,                              // x GODDESS
2,                              // x HEAVENLY_GOD
(uint8_t)RaceID::NOCTURNE,      // x RAPTOR
(uint8_t)RaceID::AVIAN,         // x DIVINE
(uint8_t)RaceID::VILE,          // x EVIL_DEMON
(uint8_t)RaceID::AVIAN,         // x WILD_BIRD
(uint8_t)RaceID::GODDESS,       // x YOMA
(uint8_t)RaceID::NATION_RULER,  // x EARTH_ELEMENT
0,                              // x REAPER
(uint8_t)RaceID::GODLY_BEAST,   // x HOLY_BEAST
(uint8_t)RaceID::HOLY_BEAST,    // x BEAST
(uint8_t)RaceID::DIVINE,        // x FAIRY
(uint8_t)RaceID::DRAGON,        // x DEMIGOD
(uint8_t)RaceID::FEMME,         // x WILDER
(uint8_t)RaceID::GODDESS,       // x DRAGON_KING
(uint8_t)RaceID::FALLEN_ANGEL,  // x NOCTURNE
(uint8_t)RaceID::DRAGON,        // x GODLY_BEAST
(uint8_t)RaceID::HAUNT,         // x FOUL
(uint8_t)RaceID::GODLY_BEAST,   // x BRUTE
0,                              // x HAUNT
(uint8_t)RaceID::HOLY_BEAST,    // x DRAGON
(uint8_t)RaceID::WILD_BIRD,     // x FALLEN_ANGEL
(uint8_t)RaceID::FALLEN_ANGEL,  // x FEMME
0,                              // x NATION_RULER
(uint8_t)RaceID::GODDESS,       // x EARTH_MOTHER
0,                              // x EVIL_DRAGON
(uint8_t)RaceID::NATION_RULER,  // x GUARDIAN
(uint8_t)RaceID::NATION_RULER,  // x DESTROYER
0                               // x TYRANT
},
// 凶鳥: RAPTOR
{
(uint8_t)RaceID::REAPER,        // x SERAPHIM
(uint8_t)RaceID::VILE,          // x ENTITY
(uint8_t)RaceID::VILE,          // x DEMON_GOD
(uint8_t)RaceID::EVIL_DRAGON,   // x VILE
0,                              // x AVIAN
(uint8_t)RaceID::FALLEN_ANGEL,  // x GODDESS
(uint8_t)RaceID::NOCTURNE,      // x HEAVENLY_GOD
0,                              // x RAPTOR
(uint8_t)RaceID::WILD_BIRD,     // x DIVINE
(uint8_t)RaceID::FOUL,          // x EVIL_DEMON
0,                              // x WILD_BIRD
(uint8_t)RaceID::EVIL_DEMON,    // x YOMA
(uint8_t)RaceID::FOUL,          // x EARTH_ELEMENT
(uint8_t)RaceID::EVIL_DRAGON,   // x REAPER
(uint8_t)RaceID::AVIAN,         // x HOLY_BEAST
(uint8_t)RaceID::DRAGON_KING,   // x BEAST
(uint8_t)RaceID::HAUNT,         // x FAIRY
(uint8_t)RaceID::HOLY_BEAST,    // x DEMIGOD
(uint8_t)RaceID::WILD_BIRD,     // x WILDER
(uint8_t)RaceID::FOUL,          // x DRAGON_KING
(uint8_t)RaceID::WILD_BIRD,     // x NOCTURNE
(uint8_t)RaceID::WILDER,        // x GODLY_BEAST
0,                              // x FOUL
(uint8_t)RaceID::FEMME,         // x BRUTE
(uint8_t)RaceID::WILD_BIRD,     // x HAUNT
(uint8_t)RaceID::WILDER,        // x DRAGON
(uint8_t)RaceID::FOUL,          // x FALLEN_ANGEL
(uint8_t)RaceID::FOUL,          // x FEMME
(uint8_t)RaceID::REAPER,        // x NATION_RULER
(uint8_t)RaceID::DRAGON_KING,   // x EARTH_MOTHER
(uint8_t)RaceID::WILDER,        // x EVIL_DRAGON
(uint8_t)RaceID::VILE,          // x GUARDIAN
(uint8_t)RaceID::EVIL_DRAGON,   // x DESTROYER
(uint8_t)RaceID::DRAGON         // x TYRANT
},
// 天使: DIVINE
{
(uint8_t)RaceID::GODDESS,       // x SERAPHIM
(uint8_t)RaceID::GODDESS,       // x ENTITY
(uint8_t)RaceID::GODDESS,       // x DEMON_GOD
(uint8_t)RaceID::REAPER,        // x VILE
(uint8_t)RaceID::DRAGON_KING,   // x AVIAN
(uint8_t)RaceID::HEAVENLY_GOD,  // x GODDESS
(uint8_t)RaceID::AVIAN,         // x HEAVENLY_GOD
(uint8_t)RaceID::WILD_BIRD,     // x RAPTOR
3,                              // x DIVINE
(uint8_t)RaceID::REAPER,        // x EVIL_DEMON
(uint8_t)RaceID::FAIRY,         // x WILD_BIRD
(uint8_t)RaceID::DRAGON_KING,   // x YOMA
(uint8_t)RaceID::NOCTURNE,      // x EARTH_ELEMENT
(uint8_t)RaceID::RAPTOR,        // x REAPER
(uint8_t)RaceID::GODDESS,       // x HOLY_BEAST
(uint8_t)RaceID::HOLY_BEAST,    // x BEAST
(uint8_t)RaceID::GODDESS,       // x FAIRY
(uint8_t)RaceID::GODDESS,       // x DEMIGOD
(uint8_t)RaceID::FALLEN_ANGEL,  // x WILDER
(uint8_t)RaceID::FAIRY,         // x DRAGON_KING
(uint8_t)RaceID::DRAGON_KING,   // x NOCTURNE
(uint8_t)RaceID::GODDESS,       // x GODLY_BEAST
(uint8_t)RaceID::FALLEN_ANGEL,  // x FOUL
(uint8_t)RaceID::YOMA,          // x BRUTE
(uint8_t)RaceID::EARTH_ELEMENT, // x HAUNT
(uint8_t)RaceID::GODDESS,       // x DRAGON
0,                              // x FALLEN_ANGEL
(uint8_t)RaceID::BEAST,         // x FEMME
(uint8_t)RaceID::WILD_BIRD,     // x NATION_RULER
(uint8_t)RaceID::GODDESS,       // x EARTH_MOTHER
(uint8_t)RaceID::WILD_BIRD,     // x EVIL_DRAGON
(uint8_t)RaceID::EARTH_MOTHER,  // x GUARDIAN
(uint8_t)RaceID::VILE,          // x DESTROYER
(uint8_t)RaceID::VILE           // x TYRANT
},
// 邪鬼: EVIL_DEMON
{
0,                              // x SERAPHIM
(uint8_t)RaceID::VILE,          // x ENTITY
0,                              // x DEMON_GOD
(uint8_t)RaceID::FOUL,          // x VILE
(uint8_t)RaceID::RAPTOR,        // x AVIAN
(uint8_t)RaceID::FEMME,         // x GODDESS
(uint8_t)RaceID::VILE,          // x HEAVENLY_GOD
(uint8_t)RaceID::FOUL,          // x RAPTOR
(uint8_t)RaceID::REAPER,        // x DIVINE
0,                              // x EVIL_DEMON
(uint8_t)RaceID::BRUTE,         // x WILD_BIRD
(uint8_t)RaceID::EARTH_ELEMENT, // x YOMA
(uint8_t)RaceID::BEAST,         // x EARTH_ELEMENT
(uint8_t)RaceID::VILE,          // x REAPER
(uint8_t)RaceID::RAPTOR,        // x HOLY_BEAST
(uint8_t)RaceID::WILDER,        // x BEAST
(uint8_t)RaceID::BRUTE,         // x FAIRY
(uint8_t)RaceID::NATION_RULER,  // x DEMIGOD
(uint8_t)RaceID::BRUTE,         // x WILDER
(uint8_t)RaceID::EVIL_DRAGON,   // x DRAGON_KING
(uint8_t)RaceID::FALLEN_ANGEL,  // x NOCTURNE
0,                              // x GODLY_BEAST
(uint8_t)RaceID::BRUTE,         // x FOUL
(uint8_t)RaceID::HAUNT,         // x BRUTE
(uint8_t)RaceID::FOUL,          // x HAUNT
(uint8_t)RaceID::GUARDIAN,      // x DRAGON
(uint8_t)RaceID::EVIL_DRAGON,   // x FALLEN_ANGEL
(uint8_t)RaceID::BEAST,         // x FEMME
(uint8_t)RaceID::VILE,          // x NATION_RULER
(uint8_t)RaceID::GUARDIAN,      // x EARTH_MOTHER
(uint8_t)RaceID::VILE,          // x EVIL_DRAGON
(uint8_t)RaceID::BRUTE,         // x GUARDIAN
0,                              // x DESTROYER
(uint8_t)RaceID::FOUL           // x TYRANT
},
// 妖鳥: WILD_BIRD
{
(uint8_t)RaceID::AVIAN,         // x SERAPHIM
(uint8_t)RaceID::AVIAN,         // x ENTITY
(uint8_t)RaceID::FALLEN_ANGEL,  // x DEMON_GOD
(uint8_t)RaceID::EVIL_DRAGON,   // x VILE
(uint8_t)RaceID::RAPTOR,        // x AVIAN
(uint8_t)RaceID::AVIAN,         // x GODDESS
(uint8_t)RaceID::AVIAN,         // x HEAVENLY_GOD
0,                              // x RAPTOR
(uint8_t)RaceID::FAIRY,         // x DIVINE
(uint8_t)RaceID::BRUTE,         // x EVIL_DEMON
3,                              // x WILD_BIRD
(uint8_t)RaceID::DIVINE,        // x YOMA
(uint8_t)RaceID::FAIRY,         // x EARTH_ELEMENT
(uint8_t)RaceID::RAPTOR,        // x REAPER
(uint8_t)RaceID::AVIAN,         // x HOLY_BEAST
(uint8_t)RaceID::WILDER,        // x BEAST
(uint8_t)RaceID::DIVINE,        // x FAIRY
(uint8_t)RaceID::AVIAN,         // x DEMIGOD
(uint8_t)RaceID::FOUL,          // x WILDER
(uint8_t)RaceID::BEAST,         // x DRAGON_KING
(uint8_t)RaceID::BRUTE,         // x NOCTURNE
(uint8_t)RaceID::AVIAN,         // x GODLY_BEAST
0,                              // x FOUL
(uint8_t)RaceID::RAPTOR,        // x BRUTE
(uint8_t)RaceID::YOMA,          // x HAUNT
(uint8_t)RaceID::HEAVENLY_GOD,  // x DRAGON
(uint8_t)RaceID::RAPTOR,        // x FALLEN_ANGEL
(uint8_t)RaceID::EVIL_DEMON,    // x FEMME
(uint8_t)RaceID::EARTH_MOTHER,  // x NATION_RULER
(uint8_t)RaceID::AVIAN,         // x EARTH_MOTHER
(uint8_t)RaceID::RAPTOR,        // x EVIL_DRAGON
(uint8_t)RaceID::FEMME,         // x GUARDIAN
(uint8_t)RaceID::EARTH_MOTHER,  // x DESTROYER
(uint8_t)RaceID::EVIL_DRAGON    // x TYRANT
},
// 妖魔: YOMA
{
(uint8_t)RaceID::GODDESS,       // x SERAPHIM
(uint8_t)RaceID::GODDESS,       // x ENTITY
(uint8_t)RaceID::DEMIGOD,       // x DEMON_GOD
(uint8_t)RaceID::EARTH_ELEMENT, // x VILE
(uint8_t)RaceID::NOCTURNE,      // x AVIAN
(uint8_t)RaceID::FEMME,         // x GODDESS
(uint8_t)RaceID::GODDESS,       // x HEAVENLY_GOD
(uint8_t)RaceID::EVIL_DEMON,    // x RAPTOR
(uint8_t)RaceID::DRAGON_KING,   // x DIVINE
(uint8_t)RaceID::EARTH_ELEMENT, // x EVIL_DEMON
(uint8_t)RaceID::DIVINE,        // x WILD_BIRD
2,                              // x YOMA
(uint8_t)RaceID::WILDER,        // x EARTH_ELEMENT
(uint8_t)RaceID::EARTH_ELEMENT, // x REAPER
(uint8_t)RaceID::DIVINE,        // x HOLY_BEAST
(uint8_t)RaceID::FALLEN_ANGEL,  // x BEAST
(uint8_t)RaceID::EARTH_ELEMENT, // x FAIRY
0,                              // x DEMIGOD
(uint8_t)RaceID::EVIL_DEMON,    // x WILDER
(uint8_t)RaceID::NOCTURNE,      // x DRAGON_KING
(uint8_t)RaceID::DIVINE,        // x NOCTURNE
(uint8_t)RaceID::DIVINE,        // x GODLY_BEAST
(uint8_t)RaceID::WILD_BIRD,     // x FOUL
(uint8_t)RaceID::FEMME,         // x BRUTE
(uint8_t)RaceID::EARTH_ELEMENT, // x HAUNT
(uint8_t)RaceID::GODLY_BEAST,   // x DRAGON
(uint8_t)RaceID::EARTH_ELEMENT, // x FALLEN_ANGEL
(uint8_t)RaceID::BRUTE,         // x FEMME
(uint8_t)RaceID::FEMME,         // x NATION_RULER
(uint8_t)RaceID::NOCTURNE,      // x EARTH_MOTHER
(uint8_t)RaceID::DRAGON_KING,   // x EVIL_DRAGON
(uint8_t)RaceID::NATION_RULER,  // x GUARDIAN
(uint8_t)RaceID::DEMIGOD,       // x DESTROYER
(uint8_t)RaceID::NOCTURNE       // x TYRANT
},
// 地霊: EARTH_ELEMENT
{
(uint8_t)RaceID::HEAVENLY_GOD,  // x SERAPHIM
(uint8_t)RaceID::NATION_RULER,  // x ENTITY
(uint8_t)RaceID::BRUTE,         // x DEMON_GOD
(uint8_t)RaceID::HAUNT,         // x VILE
(uint8_t)RaceID::DIVINE,        // x AVIAN
(uint8_t)RaceID::EARTH_MOTHER,  // x GODDESS
(uint8_t)RaceID::NATION_RULER,  // x HEAVENLY_GOD
(uint8_t)RaceID::FOUL,          // x RAPTOR
(uint8_t)RaceID::NOCTURNE,      // x DIVINE
(uint8_t)RaceID::BEAST,         // x EVIL_DEMON
(uint8_t)RaceID::FAIRY,         // x WILD_BIRD
(uint8_t)RaceID::WILDER,        // x YOMA
4,                              // x EARTH_ELEMENT
(uint8_t)RaceID::EVIL_DEMON,    // x REAPER
(uint8_t)RaceID::BEAST,         // x HOLY_BEAST
(uint8_t)RaceID::YOMA,          // x BEAST
(uint8_t)RaceID::WILDER,        // x FAIRY
(uint8_t)RaceID::EARTH_MOTHER,  // x DEMIGOD
(uint8_t)RaceID::BRUTE,         // x WILDER
(uint8_t)RaceID::FALLEN_ANGEL,  // x DRAGON_KING
(uint8_t)RaceID::FOUL,          // x NOCTURNE
(uint8_t)RaceID::NATION_RULER,  // x GODLY_BEAST
(uint8_t)RaceID::FEMME,         // x FOUL
(uint8_t)RaceID::FAIRY,         // x BRUTE
(uint8_t)RaceID::FOUL,          // x HAUNT
(uint8_t)RaceID::NATION_RULER,  // x DRAGON
(uint8_t)RaceID::YOMA,          // x FALLEN_ANGEL
(uint8_t)RaceID::WILDER,        // x FEMME
(uint8_t)RaceID::DRAGON_KING,   // x NATION_RULER
(uint8_t)RaceID::BEAST,         // x EARTH_MOTHER
(uint8_t)RaceID::EVIL_DEMON,    // x EVIL_DRAGON
(uint8_t)RaceID::DRAGON_KING,   // x GUARDIAN
(uint8_t)RaceID::FEMME,         // x DESTROYER
(uint8_t)RaceID::WILDER         // x TYRANT
},
// 死神: REAPER
{
0,                              // x SERAPHIM
(uint8_t)RaceID::VILE,          // x ENTITY
0,                              // x DEMON_GOD
(uint8_t)RaceID::FOUL,          // x VILE
(uint8_t)RaceID::RAPTOR,        // x AVIAN
(uint8_t)RaceID::VILE,          // x GODDESS
0,                              // x HEAVENLY_GOD
(uint8_t)RaceID::EVIL_DRAGON,   // x RAPTOR
(uint8_t)RaceID::RAPTOR,        // x DIVINE
(uint8_t)RaceID::VILE,          // x EVIL_DEMON
(uint8_t)RaceID::RAPTOR,        // x WILD_BIRD
(uint8_t)RaceID::EARTH_ELEMENT, // x YOMA
(uint8_t)RaceID::EVIL_DEMON,    // x EARTH_ELEMENT
0,                              // x REAPER
0,                              // x HOLY_BEAST
(uint8_t)RaceID::WILDER,        // x BEAST
(uint8_t)RaceID::NOCTURNE,      // x FAIRY
(uint8_t)RaceID::HAUNT,         // x DEMIGOD
(uint8_t)RaceID::RAPTOR,        // x WILDER
(uint8_t)RaceID::EVIL_DRAGON,   // x DRAGON_KING
(uint8_t)RaceID::FALLEN_ANGEL,  // x NOCTURNE
(uint8_t)RaceID::VILE,          // x GODLY_BEAST
0,                              // x FOUL
(uint8_t)RaceID::EARTH_ELEMENT, // x BRUTE
0,                              // x HAUNT
(uint8_t)RaceID::EVIL_DRAGON,   // x DRAGON
(uint8_t)RaceID::RAPTOR,        // x FALLEN_ANGEL
(uint8_t)RaceID::EVIL_DEMON,    // x FEMME
(uint8_t)RaceID::VILE,          // x NATION_RULER
0,                              // x EARTH_MOTHER
(uint8_t)RaceID::WILDER,        // x EVIL_DRAGON
(uint8_t)RaceID::EVIL_DEMON,    // x GUARDIAN
(uint8_t)RaceID::DRAGON,        // x DESTROYER
(uint8_t)RaceID::VILE           // x TYRANT
},
// 聖獣: HOLY_BEAST
{
(uint8_t)RaceID::GODLY_BEAST,   // x SERAPHIM
(uint8_t)RaceID::GUARDIAN,      // x ENTITY
(uint8_t)RaceID::AVIAN,         // x DEMON_GOD
0,                              // x VILE
(uint8_t)RaceID::EARTH_MOTHER,  // x AVIAN
(uint8_t)RaceID::GODLY_BEAST,   // x GODDESS
(uint8_t)RaceID::GODLY_BEAST,   // x HEAVENLY_GOD
(uint8_t)RaceID::AVIAN,         // x RAPTOR
(uint8_t)RaceID::GODDESS,       // x DIVINE
(uint8_t)RaceID::RAPTOR,        // x EVIL_DEMON
(uint8_t)RaceID::AVIAN,         // x WILD_BIRD
(uint8_t)RaceID::DIVINE,        // x YOMA
(uint8_t)RaceID::BEAST,         // x EARTH_ELEMENT
0,                              // x REAPER
1,                              // x HOLY_BEAST
(uint8_t)RaceID::GODLY_BEAST,   // x BEAST
(uint8_t)RaceID::GODDESS,       // x FAIRY
(uint8_t)RaceID::GODLY_BEAST,   // x DEMIGOD
0,                              // x WILDER
(uint8_t)RaceID::GUARDIAN,      // x DRAGON_KING
(uint8_t)RaceID::EARTH_MOTHER,  // x NOCTURNE
(uint8_t)RaceID::GODDESS,       // x GODLY_BEAST
0,                              // x FOUL
(uint8_t)RaceID::FEMME,         // x BRUTE
(uint8_t)RaceID::WILD_BIRD,     // x HAUNT
(uint8_t)RaceID::DRAGON_KING,   // x DRAGON
(uint8_t)RaceID::BEAST,         // x FALLEN_ANGEL
(uint8_t)RaceID::EARTH_MOTHER,  // x FEMME
(uint8_t)RaceID::EARTH_ELEMENT, // x NATION_RULER
(uint8_t)RaceID::GODLY_BEAST,   // x EARTH_MOTHER
0,                              // x EVIL_DRAGON
(uint8_t)RaceID::NATION_RULER,  // x GUARDIAN
(uint8_t)RaceID::GUARDIAN,      // x DESTROYER
(uint8_t)RaceID::EVIL_DRAGON    // x TYRANT
},
// 魔獣: BEAST
{
0,                              // x SERAPHIM
(uint8_t)RaceID::HOLY_BEAST,    // x ENTITY
(uint8_t)RaceID::GODLY_BEAST,   // x DEMON_GOD
(uint8_t)RaceID::FOUL,          // x VILE
(uint8_t)RaceID::DRAGON_KING,   // x AVIAN
(uint8_t)RaceID::HOLY_BEAST,    // x GODDESS
(uint8_t)RaceID::HOLY_BEAST,    // x HEAVENLY_GOD
(uint8_t)RaceID::DRAGON_KING,   // x RAPTOR
(uint8_t)RaceID::HOLY_BEAST,    // x DIVINE
(uint8_t)RaceID::WILDER,        // x EVIL_DEMON
(uint8_t)RaceID::WILDER,        // x WILD_BIRD
(uint8_t)RaceID::FALLEN_ANGEL,  // x YOMA
(uint8_t)RaceID::YOMA,          // x EARTH_ELEMENT
(uint8_t)RaceID::WILDER,        // x REAPER
(uint8_t)RaceID::GODLY_BEAST,   // x HOLY_BEAST
3,                              // x BEAST
(uint8_t)RaceID::DIVINE,        // x FAIRY
(uint8_t)RaceID::HOLY_BEAST,    // x DEMIGOD
(uint8_t)RaceID::EARTH_ELEMENT, // x WILDER
(uint8_t)RaceID::BRUTE,         // x DRAGON_KING
(uint8_t)RaceID::HOLY_BEAST,    // x NOCTURNE
(uint8_t)RaceID::DRAGON_KING,   // x GODLY_BEAST
(uint8_t)RaceID::WILDER,        // x FOUL
(uint8_t)RaceID::FEMME,         // x BRUTE
(uint8_t)RaceID::WILDER,        // x HAUNT
(uint8_t)RaceID::DRAGON_KING,   // x DRAGON
(uint8_t)RaceID::NOCTURNE,      // x FALLEN_ANGEL
(uint8_t)RaceID::FOUL,          // x FEMME
(uint8_t)RaceID::DRAGON_KING,   // x NATION_RULER
(uint8_t)RaceID::DRAGON_KING,   // x EARTH_MOTHER
(uint8_t)RaceID::REAPER,        // x EVIL_DRAGON
(uint8_t)RaceID::HOLY_BEAST,    // x GUARDIAN
(uint8_t)RaceID::EVIL_DRAGON,   // x DESTROYER
(uint8_t)RaceID::NOCTURNE       // x TYRANT
},
// 妖精: FAIRY
{
(uint8_t)RaceID::HOLY_BEAST,    // x SERAPHIM
(uint8_t)RaceID::DIVINE,        // x ENTITY
(uint8_t)RaceID::NOCTURNE,      // x DEMON_GOD
(uint8_t)RaceID::RAPTOR,        // x VILE
(uint8_t)RaceID::WILD_BIRD,     // x AVIAN
(uint8_t)RaceID::DEMIGOD,       // x GODDESS
(uint8_t)RaceID::DIVINE,        // x HEAVENLY_GOD
(uint8_t)RaceID::HAUNT,         // x RAPTOR
(uint8_t)RaceID::GODDESS,       // x DIVINE
(uint8_t)RaceID::BRUTE,         // x EVIL_DEMON
(uint8_t)RaceID::DIVINE,        // x WILD_BIRD
(uint8_t)RaceID::EARTH_ELEMENT, // x YOMA
(uint8_t)RaceID::WILDER,        // x EARTH_ELEMENT
(uint8_t)RaceID::NOCTURNE,      // x REAPER
(uint8_t)RaceID::GODDESS,       // x HOLY_BEAST
(uint8_t)RaceID::DIVINE,        // x BEAST
3,                              // x FAIRY
0,                              // x DEMIGOD
(uint8_t)RaceID::WILD_BIRD,     // x WILDER
(uint8_t)RaceID::YOMA,          // x DRAGON_KING
0,                              // x NOCTURNE
(uint8_t)RaceID::DIVINE,        // x GODLY_BEAST
(uint8_t)RaceID::HAUNT,         // x FOUL
(uint8_t)RaceID::YOMA,          // x BRUTE
(uint8_t)RaceID::WILD_BIRD,     // x HAUNT
(uint8_t)RaceID::DEMIGOD,       // x DRAGON
(uint8_t)RaceID::YOMA,          // x FALLEN_ANGEL
(uint8_t)RaceID::HAUNT,         // x FEMME
(uint8_t)RaceID::DRAGON_KING,   // x NATION_RULER
(uint8_t)RaceID::YOMA,          // x EARTH_MOTHER
(uint8_t)RaceID::DRAGON_KING,   // x EVIL_DRAGON
(uint8_t)RaceID::BRUTE,         // x GUARDIAN
(uint8_t)RaceID::BRUTE,         // x DESTROYER
(uint8_t)RaceID::NOCTURNE       // x TYRANT
},
// 幻魔: DEMIGOD
{
(uint8_t)RaceID::GODDESS,       // x SERAPHIM
(uint8_t)RaceID::GUARDIAN,      // x ENTITY
0,                              // x DEMON_GOD
(uint8_t)RaceID::NATION_RULER,  // x VILE
(uint8_t)RaceID::HEAVENLY_GOD,  // x AVIAN
(uint8_t)RaceID::HEAVENLY_GOD,  // x GODDESS
(uint8_t)RaceID::DRAGON,        // x HEAVENLY_GOD
(uint8_t)RaceID::HOLY_BEAST,    // x RAPTOR
(uint8_t)RaceID::GODDESS,       // x DIVINE
(uint8_t)RaceID::NATION_RULER,  // x EVIL_DEMON
(uint8_t)RaceID::AVIAN,         // x WILD_BIRD
0,                              // x YOMA
(uint8_t)RaceID::EARTH_MOTHER,  // x EARTH_ELEMENT
(uint8_t)RaceID::HAUNT,         // x REAPER
(uint8_t)RaceID::GODLY_BEAST,   // x HOLY_BEAST
(uint8_t)RaceID::HOLY_BEAST,    // x BEAST
0,                              // x FAIRY
0,                              // x DEMIGOD
(uint8_t)RaceID::YOMA,          // x WILDER
(uint8_t)RaceID::DRAGON,        // x DRAGON_KING
(uint8_t)RaceID::HOLY_BEAST,    // x NOCTURNE
(uint8_t)RaceID::HEAVENLY_GOD,  // x GODLY_BEAST
0,                              // x FOUL
(uint8_t)RaceID::DIVINE,        // x BRUTE
0,                              // x HAUNT
(uint8_t)RaceID::HOLY_BEAST,    // x DRAGON
(uint8_t)RaceID::EARTH_MOTHER,  // x FALLEN_ANGEL
(uint8_t)RaceID::NOCTURNE,      // x FEMME
(uint8_t)RaceID::GUARDIAN,      // x NATION_RULER
(uint8_t)RaceID::FEMME,         // x EARTH_MOTHER
(uint8_t)RaceID::GODLY_BEAST,   // x EVIL_DRAGON
(uint8_t)RaceID::GODLY_BEAST,   // x GUARDIAN
(uint8_t)RaceID::GUARDIAN,      // x DESTROYER
(uint8_t)RaceID::YOMA           // x TYRANT
},
// 妖獣: WILDER
{
0,                              // x SERAPHIM
(uint8_t)RaceID::BRUTE,         // x ENTITY
(uint8_t)RaceID::BEAST,         // x DEMON_GOD
(uint8_t)RaceID::FOUL,          // x VILE
(uint8_t)RaceID::WILD_BIRD,     // x AVIAN
(uint8_t)RaceID::BEAST,         // x GODDESS
(uint8_t)RaceID::FEMME,         // x HEAVENLY_GOD
(uint8_t)RaceID::WILD_BIRD,     // x RAPTOR
(uint8_t)RaceID::FALLEN_ANGEL,  // x DIVINE
(uint8_t)RaceID::BRUTE,         // x EVIL_DEMON
(uint8_t)RaceID::FOUL,          // x WILD_BIRD
(uint8_t)RaceID::EVIL_DEMON,    // x YOMA
(uint8_t)RaceID::BRUTE,         // x EARTH_ELEMENT
(uint8_t)RaceID::RAPTOR,        // x REAPER
0,                              // x HOLY_BEAST
(uint8_t)RaceID::EARTH_ELEMENT, // x BEAST
(uint8_t)RaceID::WILD_BIRD,     // x FAIRY
(uint8_t)RaceID::YOMA,          // x DEMIGOD
3,                              // x WILDER
(uint8_t)RaceID::NOCTURNE,      // x DRAGON_KING
(uint8_t)RaceID::BEAST,         // x NOCTURNE
0,                              // x GODLY_BEAST
(uint8_t)RaceID::WILD_BIRD,     // x FOUL
(uint8_t)RaceID::FAIRY,         // x BRUTE
(uint8_t)RaceID::EARTH_ELEMENT, // x HAUNT
(uint8_t)RaceID::BEAST,         // x DRAGON
(uint8_t)RaceID::NOCTURNE,      // x FALLEN_ANGEL
(uint8_t)RaceID::FALLEN_ANGEL,  // x FEMME
(uint8_t)RaceID::GODLY_BEAST,   // x NATION_RULER
(uint8_t)RaceID::HAUNT,         // x EARTH_MOTHER
(uint8_t)RaceID::REAPER,        // x EVIL_DRAGON
0,                              // x GUARDIAN
(uint8_t)RaceID::DRAGON,        // x DESTROYER
(uint8_t)RaceID::NOCTURNE       // x TYRANT
},
// 龍王: DRAGON_KING
{
0,                              // x SERAPHIM
(uint8_t)RaceID::HEAVENLY_GOD,  // x ENTITY
(uint8_t)RaceID::GUARDIAN,      // x DEMON_GOD
(uint8_t)RaceID::GUARDIAN,      // x VILE
(uint8_t)RaceID::HEAVENLY_GOD,  // x AVIAN
(uint8_t)RaceID::FAIRY,         // x GODDESS
(uint8_t)RaceID::GODDESS,       // x HEAVENLY_GOD
(uint8_t)RaceID::FOUL,          // x RAPTOR
(uint8_t)RaceID::FAIRY,         // x DIVINE
(uint8_t)RaceID::EVIL_DRAGON,   // x EVIL_DEMON
(uint8_t)RaceID::BEAST,         // x WILD_BIRD
(uint8_t)RaceID::NOCTURNE,      // x YOMA
(uint8_t)RaceID::FALLEN_ANGEL,  // x EARTH_ELEMENT
(uint8_t)RaceID::EVIL_DRAGON,   // x REAPER
(uint8_t)RaceID::GUARDIAN,      // x HOLY_BEAST
(uint8_t)RaceID::BRUTE,         // x BEAST
(uint8_t)RaceID::YOMA,          // x FAIRY
(uint8_t)RaceID::DRAGON,        // x DEMIGOD
(uint8_t)RaceID::NOCTURNE,      // x WILDER
2,                              // x DRAGON_KING
(uint8_t)RaceID::FALLEN_ANGEL,  // x NOCTURNE
(uint8_t)RaceID::DRAGON,        // x GODLY_BEAST
(uint8_t)RaceID::FALLEN_ANGEL,  // x FOUL
(uint8_t)RaceID::BEAST,         // x BRUTE
(uint8_t)RaceID::BRUTE,         // x HAUNT
(uint8_t)RaceID::EARTH_MOTHER,  // x DRAGON
(uint8_t)RaceID::DEMIGOD,       // x FALLEN_ANGEL
(uint8_t)RaceID::DEMIGOD,       // x FEMME
(uint8_t)RaceID::EARTH_MOTHER,  // x NATION_RULER
(uint8_t)RaceID::FEMME,         // x EARTH_MOTHER
(uint8_t)RaceID::EVIL_DEMON,    // x EVIL_DRAGON
(uint8_t)RaceID::DRAGON,        // x GUARDIAN
(uint8_t)RaceID::EVIL_DRAGON,   // x DESTROYER
(uint8_t)RaceID::EVIL_DRAGON    // x TYRANT
},
// 夜魔: NOCTURNE
{
(uint8_t)RaceID::FALLEN_ANGEL,  // x SERAPHIM
(uint8_t)RaceID::BRUTE,         // x ENTITY
(uint8_t)RaceID::VILE,          // x DEMON_GOD
(uint8_t)RaceID::HAUNT,         // x VILE
(uint8_t)RaceID::FEMME,         // x AVIAN
(uint8_t)RaceID::FALLEN_ANGEL,  // x GODDESS
(uint8_t)RaceID::FALLEN_ANGEL,  // x HEAVENLY_GOD
(uint8_t)RaceID::WILD_BIRD,     // x RAPTOR
(uint8_t)RaceID::DRAGON_KING,   // x DIVINE
(uint8_t)RaceID::FALLEN_ANGEL,  // x EVIL_DEMON
(uint8_t)RaceID::BRUTE,         // x WILD_BIRD
(uint8_t)RaceID::DIVINE,        // x YOMA
(uint8_t)RaceID::FOUL,          // x EARTH_ELEMENT
(uint8_t)RaceID::FALLEN_ANGEL,  // x REAPER
(uint8_t)RaceID::EARTH_MOTHER,  // x HOLY_BEAST
(uint8_t)RaceID::HOLY_BEAST,    // x BEAST
0,                              // x FAIRY
(uint8_t)RaceID::HOLY_BEAST,    // x DEMIGOD
(uint8_t)RaceID::BEAST,         // x WILDER
(uint8_t)RaceID::FALLEN_ANGEL,  // x DRAGON_KING
4,                              // x NOCTURNE
(uint8_t)RaceID::HOLY_BEAST,    // x GODLY_BEAST
(uint8_t)RaceID::BRUTE,         // x FOUL
(uint8_t)RaceID::GUARDIAN,      // x BRUTE
(uint8_t)RaceID::YOMA,          // x HAUNT
(uint8_t)RaceID::FEMME,         // x DRAGON
(uint8_t)RaceID::FAIRY,         // x FALLEN_ANGEL
(uint8_t)RaceID::EVIL_DEMON,    // x FEMME
(uint8_t)RaceID::EARTH_MOTHER,  // x NATION_RULER
(uint8_t)RaceID::GUARDIAN,      // x EARTH_MOTHER
(uint8_t)RaceID::FALLEN_ANGEL,  // x EVIL_DRAGON
(uint8_t)RaceID::FEMME,         // x GUARDIAN
(uint8_t)RaceID::EARTH_MOTHER,  // x DESTROYER
(uint8_t)RaceID::EARTH_MOTHER   // x TYRANT
},
// 神獣: GODLY_BEAST
{
(uint8_t)RaceID::AVIAN,         // x SERAPHIM
(uint8_t)RaceID::HEAVENLY_GOD,  // x ENTITY
(uint8_t)RaceID::DRAGON,        // x DEMON_GOD
(uint8_t)RaceID::EVIL_DRAGON,   // x VILE
(uint8_t)RaceID::HOLY_BEAST,    // x AVIAN
(uint8_t)RaceID::BEAST,         // x GODDESS
(uint8_t)RaceID::DRAGON,        // x HEAVENLY_GOD
(uint8_t)RaceID::WILDER,        // x RAPTOR
(uint8_t)RaceID::GODDESS,       // x DIVINE
0,                              // x EVIL_DEMON
(uint8_t)RaceID::AVIAN,         // x WILD_BIRD
(uint8_t)RaceID::DIVINE,        // x YOMA
(uint8_t)RaceID::NATION_RULER,  // x EARTH_ELEMENT
(uint8_t)RaceID::VILE,          // x REAPER
(uint8_t)RaceID::GODDESS,       // x HOLY_BEAST
(uint8_t)RaceID::DRAGON_KING,   // x BEAST
(uint8_t)RaceID::DIVINE,        // x FAIRY
(uint8_t)RaceID::HEAVENLY_GOD,  // x DEMIGOD
0,                              // x WILDER
(uint8_t)RaceID::DRAGON,        // x DRAGON_KING
(uint8_t)RaceID::HOLY_BEAST,    // x NOCTURNE
1,                              // x GODLY_BEAST
0,                              // x FOUL
(uint8_t)RaceID::EVIL_DRAGON,   // x BRUTE
0,                              // x HAUNT
(uint8_t)RaceID::DEMIGOD,       // x DRAGON
(uint8_t)RaceID::DIVINE,        // x FALLEN_ANGEL
(uint8_t)RaceID::GUARDIAN,      // x FEMME
(uint8_t)RaceID::DRAGON,        // x NATION_RULER
(uint8_t)RaceID::AVIAN,         // x EARTH_MOTHER
0,                              // x EVIL_DRAGON
(uint8_t)RaceID::HOLY_BEAST,    // x GUARDIAN
(uint8_t)RaceID::HOLY_BEAST,    // x DESTROYER
0                               // x TYRANT
},
// 外道: FOUL
{
(uint8_t)RaceID::FALLEN_ANGEL,  // x SERAPHIM
0,                              // x ENTITY
0,                              // x DEMON_GOD
(uint8_t)RaceID::HAUNT,         // x VILE
0,                              // x AVIAN
0,                              // x GODDESS
(uint8_t)RaceID::HAUNT,         // x HEAVENLY_GOD
0,                              // x RAPTOR
(uint8_t)RaceID::FALLEN_ANGEL,  // x DIVINE
(uint8_t)RaceID::BRUTE,         // x EVIL_DEMON
0,                              // x WILD_BIRD
(uint8_t)RaceID::WILD_BIRD,     // x YOMA
(uint8_t)RaceID::FEMME,         // x EARTH_ELEMENT
0,                              // x REAPER
0,                              // x HOLY_BEAST
(uint8_t)RaceID::WILDER,        // x BEAST
(uint8_t)RaceID::HAUNT,         // x FAIRY
0,                              // x DEMIGOD
(uint8_t)RaceID::WILD_BIRD,     // x WILDER
(uint8_t)RaceID::FALLEN_ANGEL,  // x DRAGON_KING
(uint8_t)RaceID::BRUTE,         // x NOCTURNE
0,                              // x GODLY_BEAST
0,                              // x FOUL
(uint8_t)RaceID::WILDER,        // x BRUTE
(uint8_t)RaceID::BRUTE,         // x HAUNT
(uint8_t)RaceID::DRAGON_KING,   // x DRAGON
0,                              // x FALLEN_ANGEL
(uint8_t)RaceID::WILDER,        // x FEMME
0,                              // x NATION_RULER
(uint8_t)RaceID::EARTH_ELEMENT, // x EARTH_MOTHER
(uint8_t)RaceID::VILE,          // x EVIL_DRAGON
(uint8_t)RaceID::EVIL_DRAGON,   // x GUARDIAN
0,                              // x DESTROYER
(uint8_t)RaceID::HAUNT          // x TYRANT
},
// 妖鬼: BRUTE
{
0,                              // x SERAPHIM
(uint8_t)RaceID::NATION_RULER,  // x ENTITY
(uint8_t)RaceID::GUARDIAN,      // x DEMON_GOD
(uint8_t)RaceID::FOUL,          // x VILE
(uint8_t)RaceID::DEMIGOD,       // x AVIAN
(uint8_t)RaceID::FEMME,         // x GODDESS
(uint8_t)RaceID::GODLY_BEAST,   // x HEAVENLY_GOD
(uint8_t)RaceID::FEMME,         // x RAPTOR
(uint8_t)RaceID::YOMA,          // x DIVINE
(uint8_t)RaceID::HAUNT,         // x EVIL_DEMON
(uint8_t)RaceID::RAPTOR,        // x WILD_BIRD
(uint8_t)RaceID::FEMME,         // x YOMA
(uint8_t)RaceID::FAIRY,         // x EARTH_ELEMENT
(uint8_t)RaceID::EARTH_ELEMENT, // x REAPER
(uint8_t)RaceID::FEMME,         // x HOLY_BEAST
(uint8_t)RaceID::FEMME,         // x BEAST
(uint8_t)RaceID::YOMA,          // x FAIRY
(uint8_t)RaceID::DIVINE,        // x DEMIGOD
(uint8_t)RaceID::FAIRY,         // x WILDER
(uint8_t)RaceID::BEAST,         // x DRAGON_KING
(uint8_t)RaceID::GUARDIAN,      // x NOCTURNE
(uint8_t)RaceID::EVIL_DRAGON,   // x GODLY_BEAST
(uint8_t)RaceID::WILDER,        // x FOUL
4,                              // x BRUTE
(uint8_t)RaceID::FOUL,          // x HAUNT
(uint8_t)RaceID::NOCTURNE,      // x DRAGON
(uint8_t)RaceID::EARTH_ELEMENT, // x FALLEN_ANGEL
(uint8_t)RaceID::BEAST,         // x FEMME
(uint8_t)RaceID::DRAGON_KING,   // x NATION_RULER
(uint8_t)RaceID::DIVINE,        // x EARTH_MOTHER
(uint8_t)RaceID::EVIL_DEMON,    // x EVIL_DRAGON
(uint8_t)RaceID::EARTH_ELEMENT, // x GUARDIAN
(uint8_t)RaceID::FEMME,         // x DESTROYER
(uint8_t)RaceID::HAUNT          // x TYRANT
},
// 幽鬼: HAUNT
{
(uint8_t)RaceID::FALLEN_ANGEL,  // x SERAPHIM
0,                              // x ENTITY
0,                              // x DEMON_GOD
(uint8_t)RaceID::FOUL,          // x VILE
0,                              // x AVIAN
(uint8_t)RaceID::VILE,          // x GODDESS
0,                              // x HEAVENLY_GOD
(uint8_t)RaceID::WILD_BIRD,     // x RAPTOR
(uint8_t)RaceID::EARTH_ELEMENT, // x DIVINE
(uint8_t)RaceID::FOUL,          // x EVIL_DEMON
(uint8_t)RaceID::YOMA,          // x WILD_BIRD
(uint8_t)RaceID::EARTH_ELEMENT, // x YOMA
(uint8_t)RaceID::FOUL,          // x EARTH_ELEMENT
0,                              // x REAPER
(uint8_t)RaceID::WILD_BIRD,     // x HOLY_BEAST
(uint8_t)RaceID::WILDER,        // x BEAST
(uint8_t)RaceID::WILD_BIRD,     // x FAIRY
0,                              // x DEMIGOD
(uint8_t)RaceID::EARTH_ELEMENT, // x WILDER
(uint8_t)RaceID::BRUTE,         // x DRAGON_KING
(uint8_t)RaceID::YOMA,          // x NOCTURNE
0,                              // x GODLY_BEAST
(uint8_t)RaceID::BRUTE,         // x FOUL
(uint8_t)RaceID::FOUL,          // x BRUTE
0,                              // x HAUNT
0,                              // x DRAGON
(uint8_t)RaceID::YOMA,          // x FALLEN_ANGEL
(uint8_t)RaceID::FOUL,          // x FEMME
(uint8_t)RaceID::BRUTE,         // x NATION_RULER
(uint8_t)RaceID::FEMME,         // x EARTH_MOTHER
(uint8_t)RaceID::RAPTOR,        // x EVIL_DRAGON
0,                              // x GUARDIAN
0,                              // x DESTROYER
(uint8_t)RaceID::FOUL           // x TYRANT
},
// 龍神: DRAGON
{
(uint8_t)RaceID::HOLY_BEAST,    // x SERAPHIM
(uint8_t)RaceID::EARTH_MOTHER,  // x ENTITY
(uint8_t)RaceID::NATION_RULER,  // x DEMON_GOD
(uint8_t)RaceID::DRAGON_KING,   // x VILE
(uint8_t)RaceID::GODLY_BEAST,   // x AVIAN
(uint8_t)RaceID::GODLY_BEAST,   // x GODDESS
(uint8_t)RaceID::HOLY_BEAST,    // x HEAVENLY_GOD
(uint8_t)RaceID::WILDER,        // x RAPTOR
(uint8_t)RaceID::GODDESS,       // x DIVINE
(uint8_t)RaceID::GUARDIAN,      // x EVIL_DEMON
(uint8_t)RaceID::HEAVENLY_GOD,  // x WILD_BIRD
(uint8_t)RaceID::GODLY_BEAST,   // x YOMA
(uint8_t)RaceID::NATION_RULER,  // x EARTH_ELEMENT
(uint8_t)RaceID::EVIL_DRAGON,   // x REAPER
(uint8_t)RaceID::DRAGON_KING,   // x HOLY_BEAST
(uint8_t)RaceID::DRAGON_KING,   // x BEAST
(uint8_t)RaceID::DEMIGOD,       // x FAIRY
(uint8_t)RaceID::HOLY_BEAST,    // x DEMIGOD
(uint8_t)RaceID::BEAST,         // x WILDER
(uint8_t)RaceID::EARTH_MOTHER,  // x DRAGON_KING
(uint8_t)RaceID::FEMME,         // x NOCTURNE
(uint8_t)RaceID::DEMIGOD,       // x GODLY_BEAST
(uint8_t)RaceID::DRAGON_KING,   // x FOUL
(uint8_t)RaceID::NOCTURNE,      // x BRUTE
0,                              // x HAUNT
2,                              // x DRAGON
(uint8_t)RaceID::DRAGON_KING,   // x FALLEN_ANGEL
(uint8_t)RaceID::NOCTURNE,      // x FEMME
(uint8_t)RaceID::EARTH_MOTHER,  // x NATION_RULER
0,                              // x EARTH_MOTHER
0,                              // x EVIL_DRAGON
(uint8_t)RaceID::NATION_RULER,  // x GUARDIAN
(uint8_t)RaceID::EVIL_DRAGON,   // x DESTROYER
(uint8_t)RaceID::EVIL_DRAGON    // x TYRANT
},
// 堕天使: FALLEN_ANGEL
{
(uint8_t)RaceID::EARTH_MOTHER,  // x SERAPHIM
(uint8_t)RaceID::GUARDIAN,      // x ENTITY
0,                              // x DEMON_GOD
(uint8_t)RaceID::BRUTE,         // x VILE
(uint8_t)RaceID::DRAGON_KING,   // x AVIAN
(uint8_t)RaceID::DIVINE,        // x GODDESS
(uint8_t)RaceID::WILD_BIRD,     // x HEAVENLY_GOD
(uint8_t)RaceID::FOUL,          // x RAPTOR
0,                              // x DIVINE
(uint8_t)RaceID::EVIL_DRAGON,   // x EVIL_DEMON
(uint8_t)RaceID::RAPTOR,        // x WILD_BIRD
(uint8_t)RaceID::EARTH_ELEMENT, // x YOMA
(uint8_t)RaceID::YOMA,          // x EARTH_ELEMENT
(uint8_t)RaceID::RAPTOR,        // x REAPER
(uint8_t)RaceID::BEAST,         // x HOLY_BEAST
(uint8_t)RaceID::NOCTURNE,      // x BEAST
(uint8_t)RaceID::YOMA,          // x FAIRY
(uint8_t)RaceID::EARTH_MOTHER,  // x DEMIGOD
(uint8_t)RaceID::NOCTURNE,      // x WILDER
(uint8_t)RaceID::DEMIGOD,       // x DRAGON_KING
(uint8_t)RaceID::FAIRY,         // x NOCTURNE
(uint8_t)RaceID::DIVINE,        // x GODLY_BEAST
0,                              // x FOUL
(uint8_t)RaceID::EARTH_ELEMENT, // x BRUTE
(uint8_t)RaceID::YOMA,          // x HAUNT
(uint8_t)RaceID::DRAGON_KING,   // x DRAGON
4,                              // x FALLEN_ANGEL
(uint8_t)RaceID::WILDER,        // x FEMME
(uint8_t)RaceID::DRAGON_KING,   // x NATION_RULER
(uint8_t)RaceID::FEMME,         // x EARTH_MOTHER
(uint8_t)RaceID::EVIL_DEMON,    // x EVIL_DRAGON
(uint8_t)RaceID::NOCTURNE,      // x GUARDIAN
(uint8_t)RaceID::VILE,          // x DESTROYER
0                               // x TYRANT
},
// 鬼女: FEMME
{
0,                              // x SERAPHIM
(uint8_t)RaceID::EARTH_MOTHER,  // x ENTITY
(uint8_t)RaceID::EARTH_MOTHER,  // x DEMON_GOD
(uint8_t)RaceID::BRUTE,         // x VILE
(uint8_t)RaceID::GODDESS,       // x AVIAN
(uint8_t)RaceID::EARTH_MOTHER,  // x GODDESS
(uint8_t)RaceID::FALLEN_ANGEL,  // x HEAVENLY_GOD
(uint8_t)RaceID::FOUL,          // x RAPTOR
(uint8_t)RaceID::BEAST,         // x DIVINE
(uint8_t)RaceID::BEAST,         // x EVIL_DEMON
(uint8_t)RaceID::EVIL_DEMON,    // x WILD_BIRD
(uint8_t)RaceID::BRUTE,         // x YOMA
(uint8_t)RaceID::WILDER,        // x EARTH_ELEMENT
(uint8_t)RaceID::EVIL_DEMON,    // x REAPER
(uint8_t)RaceID::EARTH_MOTHER,  // x HOLY_BEAST
(uint8_t)RaceID::FOUL,          // x BEAST
(uint8_t)RaceID::HAUNT,         // x FAIRY
(uint8_t)RaceID::NOCTURNE,      // x DEMIGOD
(uint8_t)RaceID::FALLEN_ANGEL,  // x WILDER
(uint8_t)RaceID::DEMIGOD,       // x DRAGON_KING
(uint8_t)RaceID::EVIL_DEMON,    // x NOCTURNE
(uint8_t)RaceID::GUARDIAN,      // x GODLY_BEAST
(uint8_t)RaceID::WILDER,        // x FOUL
(uint8_t)RaceID::BEAST,         // x BRUTE
(uint8_t)RaceID::FOUL,          // x HAUNT
(uint8_t)RaceID::NOCTURNE,      // x DRAGON
(uint8_t)RaceID::WILDER,        // x FALLEN_ANGEL
2,                              // x FEMME
(uint8_t)RaceID::GUARDIAN,      // x NATION_RULER
(uint8_t)RaceID::GUARDIAN,      // x EARTH_MOTHER
(uint8_t)RaceID::EVIL_DEMON,    // x EVIL_DRAGON
(uint8_t)RaceID::EARTH_MOTHER,  // x GUARDIAN
0,                              // x DESTROYER
(uint8_t)RaceID::EARTH_MOTHER   // x TYRANT
},
// 国津神: NATION_RULER
{
(uint8_t)RaceID::DRAGON,        // x SERAPHIM
(uint8_t)RaceID::DRAGON,        // x ENTITY
(uint8_t)RaceID::HEAVENLY_GOD,  // x DEMON_GOD
(uint8_t)RaceID::REAPER,        // x VILE
(uint8_t)RaceID::HEAVENLY_GOD,  // x AVIAN
(uint8_t)RaceID::EARTH_MOTHER,  // x GODDESS
0,                              // x HEAVENLY_GOD
(uint8_t)RaceID::REAPER,        // x RAPTOR
(uint8_t)RaceID::WILD_BIRD,     // x DIVINE
(uint8_t)RaceID::VILE,          // x EVIL_DEMON
(uint8_t)RaceID::EARTH_MOTHER,  // x WILD_BIRD
(uint8_t)RaceID::FEMME,         // x YOMA
(uint8_t)RaceID::DRAGON_KING,   // x EARTH_ELEMENT
(uint8_t)RaceID::VILE,          // x REAPER
(uint8_t)RaceID::EARTH_ELEMENT, // x HOLY_BEAST
(uint8_t)RaceID::DRAGON_KING,   // x BEAST
(uint8_t)RaceID::DRAGON_KING,   // x FAIRY
(uint8_t)RaceID::GUARDIAN,      // x DEMIGOD
(uint8_t)RaceID::GODLY_BEAST,   // x WILDER
(uint8_t)RaceID::EARTH_MOTHER,  // x DRAGON_KING
(uint8_t)RaceID::EARTH_MOTHER,  // x NOCTURNE
(uint8_t)RaceID::DRAGON,        // x GODLY_BEAST
0,                              // x FOUL
(uint8_t)RaceID::DRAGON_KING,   // x BRUTE
(uint8_t)RaceID::BRUTE,         // x HAUNT
(uint8_t)RaceID::EARTH_MOTHER,  // x DRAGON
(uint8_t)RaceID::DRAGON_KING,   // x FALLEN_ANGEL
(uint8_t)RaceID::GUARDIAN,      // x FEMME
4,                              // x NATION_RULER
(uint8_t)RaceID::GUARDIAN,      // x EARTH_MOTHER
(uint8_t)RaceID::REAPER,        // x EVIL_DRAGON
0,                              // x GUARDIAN
(uint8_t)RaceID::DRAGON,        // x DESTROYER
(uint8_t)RaceID::GUARDIAN       // x TYRANT
},
// 地母神: EARTH_MOTHER
{
(uint8_t)RaceID::HEAVENLY_GOD,  // x SERAPHIM
(uint8_t)RaceID::HEAVENLY_GOD,  // x ENTITY
0,                              // x DEMON_GOD
(uint8_t)RaceID::REAPER,        // x VILE
(uint8_t)RaceID::DRAGON,        // x AVIAN
0,                              // x GODDESS
(uint8_t)RaceID::GODDESS,       // x HEAVENLY_GOD
(uint8_t)RaceID::DRAGON_KING,   // x RAPTOR
(uint8_t)RaceID::GODDESS,       // x DIVINE
(uint8_t)RaceID::GUARDIAN,      // x EVIL_DEMON
(uint8_t)RaceID::AVIAN,         // x WILD_BIRD
(uint8_t)RaceID::NOCTURNE,      // x YOMA
(uint8_t)RaceID::BEAST,         // x EARTH_ELEMENT
0,                              // x REAPER
(uint8_t)RaceID::GODLY_BEAST,   // x HOLY_BEAST
(uint8_t)RaceID::DRAGON_KING,   // x BEAST
(uint8_t)RaceID::YOMA,          // x FAIRY
(uint8_t)RaceID::FEMME,         // x DEMIGOD
(uint8_t)RaceID::HAUNT,         // x WILDER
(uint8_t)RaceID::FEMME,         // x DRAGON_KING
(uint8_t)RaceID::GUARDIAN,      // x NOCTURNE
(uint8_t)RaceID::AVIAN,         // x GODLY_BEAST
(uint8_t)RaceID::EARTH_ELEMENT, // x FOUL
(uint8_t)RaceID::DIVINE,        // x BRUTE
(uint8_t)RaceID::FEMME,         // x HAUNT
0,                              // x DRAGON
(uint8_t)RaceID::FEMME,         // x FALLEN_ANGEL
(uint8_t)RaceID::GUARDIAN,      // x FEMME
(uint8_t)RaceID::GUARDIAN,      // x NATION_RULER
4,    // x EARTH_MOTHER
(uint8_t)RaceID::EARTH_ELEMENT, // x EVIL_DRAGON
(uint8_t)RaceID::FEMME,         // x GUARDIAN
(uint8_t)RaceID::VILE,          // x DESTROYER
0                               // x TYRANT
},
// 邪龍: EVIL_DRAGON
{
0,                              // x SERAPHIM
0,                              // x ENTITY
0,                              // x DEMON_GOD
(uint8_t)RaceID::WILDER,        // x VILE
0,                              // x AVIAN
(uint8_t)RaceID::VILE,          // x GODDESS
0,                              // x HEAVENLY_GOD
(uint8_t)RaceID::WILDER,        // x RAPTOR
(uint8_t)RaceID::WILD_BIRD,     // x DIVINE
(uint8_t)RaceID::VILE,          // x EVIL_DEMON
(uint8_t)RaceID::RAPTOR,        // x WILD_BIRD
(uint8_t)RaceID::DRAGON_KING,   // x YOMA
(uint8_t)RaceID::EVIL_DEMON,    // x EARTH_ELEMENT
(uint8_t)RaceID::WILDER,        // x REAPER
0,                              // x HOLY_BEAST
(uint8_t)RaceID::REAPER,        // x BEAST
(uint8_t)RaceID::DRAGON_KING,   // x FAIRY
(uint8_t)RaceID::GODLY_BEAST,   // x DEMIGOD
(uint8_t)RaceID::REAPER,        // x WILDER
(uint8_t)RaceID::EVIL_DEMON,    // x DRAGON_KING
(uint8_t)RaceID::FALLEN_ANGEL,  // x NOCTURNE
0,                              // x GODLY_BEAST
(uint8_t)RaceID::VILE,          // x FOUL
(uint8_t)RaceID::EVIL_DEMON,    // x BRUTE
(uint8_t)RaceID::RAPTOR,        // x HAUNT
0,                              // x DRAGON
(uint8_t)RaceID::EVIL_DEMON,    // x FALLEN_ANGEL
(uint8_t)RaceID::EVIL_DEMON,    // x FEMME
(uint8_t)RaceID::REAPER,        // x NATION_RULER
(uint8_t)RaceID::EARTH_ELEMENT, // x EARTH_MOTHER
0,                              // x EVIL_DRAGON
(uint8_t)RaceID::VILE,          // x GUARDIAN
0,                              // x DESTROYER
(uint8_t)RaceID::VILE           // x TYRANT
},
// 鬼神: GUARDIAN
{
(uint8_t)RaceID::NATION_RULER,  // x SERAPHIM
(uint8_t)RaceID::GODDESS,       // x ENTITY
(uint8_t)RaceID::HEAVENLY_GOD,  // x DEMON_GOD
(uint8_t)RaceID::REAPER,        // x VILE
(uint8_t)RaceID::DRAGON,        // x AVIAN
(uint8_t)RaceID::EARTH_MOTHER,  // x GODDESS
(uint8_t)RaceID::NATION_RULER,  // x HEAVENLY_GOD
(uint8_t)RaceID::VILE,          // x RAPTOR
(uint8_t)RaceID::EARTH_MOTHER,  // x DIVINE
(uint8_t)RaceID::BRUTE,         // x EVIL_DEMON
(uint8_t)RaceID::FEMME,         // x WILD_BIRD
(uint8_t)RaceID::NATION_RULER,  // x YOMA
(uint8_t)RaceID::DRAGON_KING,   // x EARTH_ELEMENT
(uint8_t)RaceID::EVIL_DEMON,    // x REAPER
(uint8_t)RaceID::NATION_RULER,  // x HOLY_BEAST
(uint8_t)RaceID::HOLY_BEAST,    // x BEAST
(uint8_t)RaceID::BRUTE,         // x FAIRY
(uint8_t)RaceID::GODLY_BEAST,   // x DEMIGOD
0,                              // x WILDER
(uint8_t)RaceID::DRAGON,        // x DRAGON_KING
(uint8_t)RaceID::FEMME,         // x NOCTURNE
(uint8_t)RaceID::HOLY_BEAST,    // x GODLY_BEAST
(uint8_t)RaceID::EVIL_DRAGON,   // x FOUL
(uint8_t)RaceID::EARTH_ELEMENT, // x BRUTE
0,                              // x HAUNT
(uint8_t)RaceID::NATION_RULER,  // x DRAGON
(uint8_t)RaceID::NOCTURNE,      // x FALLEN_ANGEL
(uint8_t)RaceID::EARTH_MOTHER,  // x FEMME
0,                              // x NATION_RULER
(uint8_t)RaceID::FEMME,         // x EARTH_MOTHER
(uint8_t)RaceID::VILE,          // x EVIL_DRAGON
0,                              // x GUARDIAN
(uint8_t)RaceID::EARTH_MOTHER,  // x DESTROYER
(uint8_t)RaceID::DRAGON         // x TYRANT
},
// 破壊神: DESTROYER
{
(uint8_t)RaceID::VILE,          // x SERAPHIM
(uint8_t)RaceID::EARTH_MOTHER,  // x ENTITY
0,                              // x DEMON_GOD
(uint8_t)RaceID::REAPER,        // x VILE
(uint8_t)RaceID::GUARDIAN,      // x AVIAN
(uint8_t)RaceID::FEMME,         // x GODDESS
(uint8_t)RaceID::NATION_RULER,  // x HEAVENLY_GOD
(uint8_t)RaceID::EVIL_DRAGON,   // x RAPTOR
(uint8_t)RaceID::VILE,          // x DIVINE
0,                              // x EVIL_DEMON
(uint8_t)RaceID::EARTH_MOTHER,  // x WILD_BIRD
(uint8_t)RaceID::DEMIGOD,       // x YOMA
(uint8_t)RaceID::FEMME,         // x EARTH_ELEMENT
(uint8_t)RaceID::DRAGON,        // x REAPER
(uint8_t)RaceID::GUARDIAN,      // x HOLY_BEAST
(uint8_t)RaceID::EVIL_DRAGON,   // x BEAST
(uint8_t)RaceID::BRUTE,         // x FAIRY
(uint8_t)RaceID::GUARDIAN,      // x DEMIGOD
(uint8_t)RaceID::DRAGON,        // x WILDER
(uint8_t)RaceID::EVIL_DRAGON,   // x DRAGON_KING
(uint8_t)RaceID::EARTH_MOTHER,  // x NOCTURNE
(uint8_t)RaceID::HOLY_BEAST,    // x GODLY_BEAST
0,                              // x FOUL
(uint8_t)RaceID::FEMME,         // x BRUTE
0,                              // x HAUNT
(uint8_t)RaceID::EVIL_DRAGON,   // x DRAGON
(uint8_t)RaceID::VILE,          // x FALLEN_ANGEL
0,                              // x FEMME
(uint8_t)RaceID::DRAGON,        // x NATION_RULER
(uint8_t)RaceID::VILE,          // x EARTH_MOTHER
0,                              // x EVIL_DRAGON
(uint8_t)RaceID::EARTH_MOTHER,  // x GUARDIAN
0,                              // x DESTROYER
(uint8_t)RaceID::EARTH_MOTHER   // x TYRANT
},
// 魔王: TYRANT
{
(uint8_t)RaceID::FALLEN_ANGEL,  // x SERAPHIM
(uint8_t)RaceID::VILE,          // x ENTITY
0,                              // x DEMON_GOD
(uint8_t)RaceID::REAPER,        // x VILE
0,                              // x AVIAN
0,                              // x GODDESS
0,                              // x HEAVENLY_GOD
(uint8_t)RaceID::DRAGON,        // x RAPTOR
(uint8_t)RaceID::VILE,          // x DIVINE
(uint8_t)RaceID::FOUL,          // x EVIL_DEMON
(uint8_t)RaceID::EVIL_DRAGON,   // x WILD_BIRD
(uint8_t)RaceID::NOCTURNE,      // x YOMA
(uint8_t)RaceID::WILDER,        // x EARTH_ELEMENT
(uint8_t)RaceID::VILE,          // x REAPER
(uint8_t)RaceID::EVIL_DRAGON,   // x HOLY_BEAST
(uint8_t)RaceID::NOCTURNE,      // x BEAST
(uint8_t)RaceID::NOCTURNE,      // x FAIRY
(uint8_t)RaceID::YOMA,          // x DEMIGOD
(uint8_t)RaceID::NOCTURNE,      // x WILDER
(uint8_t)RaceID::EVIL_DRAGON,   // x DRAGON_KING
(uint8_t)RaceID::EARTH_MOTHER,  // x NOCTURNE
0,                              // x GODLY_BEAST
(uint8_t)RaceID::HAUNT,         // x FOUL
(uint8_t)RaceID::HAUNT,         // x BRUTE
(uint8_t)RaceID::FOUL,          // x HAUNT
(uint8_t)RaceID::EVIL_DRAGON,   // x DRAGON
0,                              // x FALLEN_ANGEL
(uint8_t)RaceID::EARTH_MOTHER,  // x FEMME
(uint8_t)RaceID::GUARDIAN,      // x NATION_RULER
0,                              // x EARTH_MOTHER
(uint8_t)RaceID::VILE,          // x EVIL_DRAGON
(uint8_t)RaceID::DRAGON,        // x GUARDIAN
(uint8_t)RaceID::EARTH_MOTHER,  // x DESTROYER
0                               // x TYRANT
}
};

int8_t FUSION_ELEMENTAL_ADJUST[34][4] =
{
    { 0, 0, 0, 0 },     // SERAPHIM
    { 0, 0, 0, 0 },     // ENTITY
    { 0, 0, 0, 0 },     // DEMON_GOD
    { -1, -1, -1, -1 }, // VILE
    { -1, -1, -1, -1 }, // AVIAN
    { -1, -1, -1, -1 }, // GODDESS
    { -1, -1, -1, -1 }, // HEAVENLY_GOD
    { -1, -1, -1, -1 }, // RAPTOR
    { 1, 1, -1, -1 },   // DIVINE
    { -1, -1, -1, -1 }, // EVIL_DEMON
    { -1, -1, -1, -1 }, // WILD_BIRD
    { -1, 1, 1, -1 },   // YOMA
    { -1, -1, 1, 1 },   // EARTH_ELEMENT
    { -1, -1, -1, -1 }, // REAPER
    { 1, -1, -1, -1 },  // HOLY_BEAST
    { 1, -1, 1, -1 },   // BEAST
    { -1, 1, -1, 1 },   // FAIRY
    { -1, -1, -1, -1 }, // DEMIGOD
    { 1, 1, -1, -1 },   // WILDER
    { 1, 1, -1, -1 },   // DRAGON_KING
    { -1, -1, 1, -1 },  // NOCTURNE
    { -1, -1, -1, -1 }, // GODLY_BEAST
    { -1, 1, -1, -1 },  // FOUL
    { 1, 1, -1, 1 },    // BRUTE
    { -1, -1, 1, -1 },  // HAUNT 
    { -1, -1, -1, -1 }, // DRAGON
    { 1, -1, 1, -1 },   // FALLEN_ANGEL
    { 1, 1, -1, 1 },    // FEMME
    { -1, -1, -1, -1 }, // NATION_RULER
    { -1, -1, -1, 1 },  // EARTH_MOTHER
    { -1, -1, -1, -1 }, // EVIL_DRAGON
    { -1, -1, -1, 1 },  // GUARDIAN
    { 0, 0, 0, 0 },     // DESTROYER
    { 0, 0, 0, 0 }      // TYRANT
};

uint8_t FUSION_FAMILIARITY_BONUS[5][5] =
{
    { 1, 0, 0, 1, 3 },
    { 21, 0, 1, 2, 4 },
    { 51, 0, 1, 2, 5 },
    { 71, 1, 1, 3, 6 },
    { 91, 1, 1, 3, 7 }
};

uint8_t TRIFUSION_RACE_PRIORITY[34] =
{
    (uint8_t)RaceID::SERAPHIM,
    (uint8_t)RaceID::HEAVENLY_GOD,
    (uint8_t)RaceID::AVIAN,
    (uint8_t)RaceID::GODDESS,
    (uint8_t)RaceID::DEMON_GOD,
    (uint8_t)RaceID::GODLY_BEAST,
    (uint8_t)RaceID::HOLY_BEAST,
    (uint8_t)RaceID::VILE,
    (uint8_t)RaceID::ELEMENTAL,
    (uint8_t)RaceID::DESTROYER,
    (uint8_t)RaceID::DRAGON,
    (uint8_t)RaceID::EARTH_MOTHER,
    (uint8_t)RaceID::NATION_RULER,
    (uint8_t)RaceID::GUARDIAN,
    (uint8_t)RaceID::DEMIGOD,
    (uint8_t)RaceID::TYRANT,
    (uint8_t)RaceID::DIVINE,
    (uint8_t)RaceID::REAPER,
    (uint8_t)RaceID::RAPTOR,
    (uint8_t)RaceID::WILD_BIRD,
    (uint8_t)RaceID::DRAGON_KING,
    (uint8_t)RaceID::YOMA,
    (uint8_t)RaceID::BEAST,
    (uint8_t)RaceID::NOCTURNE,
    (uint8_t)RaceID::EARTH_ELEMENT,
    (uint8_t)RaceID::FAIRY,
    (uint8_t)RaceID::EVIL_DEMON,
    (uint8_t)RaceID::FALLEN_ANGEL,
    (uint8_t)RaceID::EVIL_DRAGON,
    (uint8_t)RaceID::WILDER,
    (uint8_t)RaceID::BRUTE,
    (uint8_t)RaceID::FEMME,
    (uint8_t)RaceID::HAUNT,
    (uint8_t)RaceID::FOUL
};


uint8_t TRIFUSION_FAMILY_MAP[7][7][8] =
{
// 神族: GOD
{
    // x AERIAL
    {
        (uint8_t)RaceID::GODDESS,       // x GOD
        (uint8_t)RaceID::GODDESS,       // x AERIAL
        (uint8_t)RaceID::EARTH_MOTHER,  // x GUARDIAN
        (uint8_t)RaceID::GUARDIAN,      // x DEMONIAC
        (uint8_t)RaceID::REAPER,        // x DRAGON
        (uint8_t)RaceID::GODDESS,       // x MAGICA
        (uint8_t)RaceID::HEAVENLY_GOD,  // x BIRD
        (uint8_t)RaceID::DEMIGOD        // x BEAST
    },
    // x GUARDIAN
    {
        (uint8_t)RaceID::HEAVENLY_GOD,  // x GOD
        (uint8_t)RaceID::GODDESS,       // x AERIAL
        (uint8_t)RaceID::GUARDIAN,      // x GUARDIAN
        (uint8_t)RaceID::GUARDIAN,      // x DEMONIAC
        (uint8_t)RaceID::DRAGON,        // x DRAGON
        (uint8_t)RaceID::DEMIGOD,       // x MAGICA
        (uint8_t)RaceID::DIVINE,        // x BIRD
        (uint8_t)RaceID::VILE           // x BEAST
    },
    // x DEMONIAC
    {
        (uint8_t)RaceID::GODLY_BEAST,   // x GOD
        (uint8_t)RaceID::EARTH_MOTHER,  // x AERIAL
        (uint8_t)RaceID::NATION_RULER,  // x GUARDIAN
        (uint8_t)RaceID::DEMIGOD,       // x DEMONIAC
        (uint8_t)RaceID::GODLY_BEAST,   // x DRAGON
        (uint8_t)RaceID::DEMIGOD,       // x MAGICA
        (uint8_t)RaceID::FEMME,         // x BIRD
        (uint8_t)RaceID::HOLY_BEAST     // x BEAST
    },
    // x DRAGON
    {
        (uint8_t)RaceID::NATION_RULER,  // x GOD
        (uint8_t)RaceID::VILE,          // x AERIAL
        (uint8_t)RaceID::REAPER,        // x GUARDIAN
        (uint8_t)RaceID::EARTH_MOTHER,  // x DEMONIAC
        (uint8_t)RaceID::DRAGON,        // x DRAGON
        (uint8_t)RaceID::DRAGON,        // x MAGICA
        (uint8_t)RaceID::GODLY_BEAST,   // x BIRD
        (uint8_t)RaceID::DIVINE         // x BEAST
    },
    // x MAGICA
    {
        (uint8_t)RaceID::GODDESS,       // x GOD
        (uint8_t)RaceID::HEAVENLY_GOD,  // x AERIAL
        (uint8_t)RaceID::DEMIGOD,       // x GUARDIAN
        (uint8_t)RaceID::VILE,          // x DEMONIAC
        (uint8_t)RaceID::DEMIGOD,       // x DRAGON
        (uint8_t)RaceID::EARTH_MOTHER,  // x MAGICA
        (uint8_t)RaceID::GODDESS,       // x BIRD
        (uint8_t)RaceID::GODLY_BEAST    // x BEAST
    },
    // x BIRD
    {
        (uint8_t)RaceID::GUARDIAN,      // x GOD
        (uint8_t)RaceID::GODDESS,       // x AERIAL
        (uint8_t)RaceID::EARTH_MOTHER,  // x GUARDIAN
        (uint8_t)RaceID::HOLY_BEAST,    // x DEMONIAC
        (uint8_t)RaceID::HEAVENLY_GOD,  // x DRAGON
        (uint8_t)RaceID::HEAVENLY_GOD,  // x MAGICA
        (uint8_t)RaceID::AVIAN,         // x BIRD
        (uint8_t)RaceID::FALLEN_ANGEL   // x BEAST
    },
    // x BEAST
    {
        (uint8_t)RaceID::VILE,          // x GOD
        (uint8_t)RaceID::AVIAN,         // x AERIAL
        (uint8_t)RaceID::FALLEN_ANGEL,  // x GUARDIAN
        (uint8_t)RaceID::WILD_BIRD,     // x DEMONIAC
        (uint8_t)RaceID::RAPTOR,        // x DRAGON
        (uint8_t)RaceID::BEAST,         // x MAGICA
        (uint8_t)RaceID::DIVINE,        // x BIRD
        (uint8_t)RaceID::VILE           // x BEAST
    }
},
// 飛天族 AERIAL
{
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    // x GUARDIAN
    {
        (uint8_t)RaceID::REAPER,        // x GOD
        (uint8_t)RaceID::NOCTURNE,      // x AERIAL
        (uint8_t)RaceID::NATION_RULER,  // x GUARDIAN
        (uint8_t)RaceID::WILD_BIRD,     // x DEMONIAC
        (uint8_t)RaceID::RAPTOR,        // x DRAGON
        (uint8_t)RaceID::VILE,          // x MAGICA
        (uint8_t)RaceID::FAIRY,         // x BIRD
        (uint8_t)RaceID::EARTH_ELEMENT  // x BEAST
    },
    // x DEMONIAC
    {
        (uint8_t)RaceID::VILE,          // x GOD
        (uint8_t)RaceID::HOLY_BEAST,    // x AERIAL
        (uint8_t)RaceID::BEAST,         // x GUARDIAN
        (uint8_t)RaceID::FOUL,          // x DEMONIAC
        (uint8_t)RaceID::FAIRY,         // x DRAGON
        (uint8_t)RaceID::RAPTOR,        // x MAGICA
        (uint8_t)RaceID::NOCTURNE,      // x BIRD
        (uint8_t)RaceID::EARTH_ELEMENT  // x BEAST
    },
    // x DRAGON
    {
        (uint8_t)RaceID::VILE,          // x GOD
        (uint8_t)RaceID::HOLY_BEAST,    // x AERIAL
        (uint8_t)RaceID::DRAGON_KING,   // x GUARDIAN
        (uint8_t)RaceID::YOMA,          // x DEMONIAC
        (uint8_t)RaceID::BEAST,         // x DRAGON
        (uint8_t)RaceID::EVIL_DRAGON,   // x MAGICA
        (uint8_t)RaceID::FALLEN_ANGEL,  // x BIRD
        (uint8_t)RaceID::DRAGON_KING    // x BEAST
    },
    // x MAGICA
    {
        (uint8_t)RaceID::GODDESS,       // x GOD
        (uint8_t)RaceID::HEAVENLY_GOD,  // x AERIAL
        (uint8_t)RaceID::NATION_RULER,  // x GUARDIAN
        (uint8_t)RaceID::NOCTURNE,      // x DEMONIAC
        (uint8_t)RaceID::GODLY_BEAST,   // x DRAGON
        (uint8_t)RaceID::REAPER,        // x MAGICA
        (uint8_t)RaceID::AVIAN,         // x BIRD
        (uint8_t)RaceID::HOLY_BEAST     // x BEAST
    },
    // x BIRD
    {
        (uint8_t)RaceID::GODDESS,       // x GOD
        (uint8_t)RaceID::GODDESS,       // x AERIAL
        (uint8_t)RaceID::GODLY_BEAST,   // x GUARDIAN
        (uint8_t)RaceID::BEAST,         // x DEMONIAC
        (uint8_t)RaceID::AVIAN,         // x DRAGON
        (uint8_t)RaceID::GODLY_BEAST,   // x MAGICA
        (uint8_t)RaceID::GODDESS,       // x BIRD
        (uint8_t)RaceID::WILD_BIRD      // x BEAST
    },
    // x BEAST
    {
        (uint8_t)RaceID::AVIAN,         // x GOD
        (uint8_t)RaceID::AVIAN,         // x AERIAL
        (uint8_t)RaceID::DEMIGOD,       // x GUARDIAN
        (uint8_t)RaceID::EVIL_DEMON,    // x DEMONIAC
        (uint8_t)RaceID::FEMME,         // x DRAGON
        (uint8_t)RaceID::HOLY_BEAST,    // x MAGICA
        (uint8_t)RaceID::GODLY_BEAST,   // x BIRD
        (uint8_t)RaceID::HOLY_BEAST     // x BEAST
    }
},
// 鬼神族 GUARDIAN
{
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    // x DEMONIAC
    {
        (uint8_t)RaceID::NATION_RULER,  // x GOD
        (uint8_t)RaceID::EVIL_DEMON,    // x AERIAL
        (uint8_t)RaceID::BEAST,         // x GUARDIAN
        (uint8_t)RaceID::BRUTE,         // x DEMONIAC
        (uint8_t)RaceID::VILE,          // x DRAGON
        (uint8_t)RaceID::DRAGON_KING,   // x MAGICA
        (uint8_t)RaceID::WILDER,        // x BIRD
        (uint8_t)RaceID::EARTH_ELEMENT  // x BEAST
    },
    // x DRAGON
    {
        (uint8_t)RaceID::VILE,          // x GOD
        (uint8_t)RaceID::EARTH_MOTHER,  // x AERIAL
        (uint8_t)RaceID::GUARDIAN,      // x GUARDIAN
        (uint8_t)RaceID::GUARDIAN,      // x DEMONIAC
        (uint8_t)RaceID::DRAGON,        // x DRAGON
        (uint8_t)RaceID::DRAGON_KING,   // x MAGICA
        (uint8_t)RaceID::FEMME,         // x BIRD
        (uint8_t)RaceID::EVIL_DRAGON    // x BEAST
    },
    // x MAGICA
    {
        (uint8_t)RaceID::EARTH_MOTHER,  // x GOD
        (uint8_t)RaceID::NATION_RULER,  // x AERIAL
        (uint8_t)RaceID::REAPER,        // x GUARDIAN
        (uint8_t)RaceID::GUARDIAN,      // x DEMONIAC
        (uint8_t)RaceID::FEMME,         // x DRAGON
        (uint8_t)RaceID::EARTH_MOTHER,  // x MAGICA
        (uint8_t)RaceID::NOCTURNE,      // x BIRD
        (uint8_t)RaceID::GUARDIAN       // x BEAST
    },
    // x BIRD
    {
        (uint8_t)RaceID::EARTH_MOTHER,  // x GOD
        (uint8_t)RaceID::RAPTOR,        // x AERIAL
        (uint8_t)RaceID::NATION_RULER,  // x GUARDIAN
        (uint8_t)RaceID::EARTH_ELEMENT, // x DEMONIAC
        (uint8_t)RaceID::NATION_RULER,  // x DRAGON
        (uint8_t)RaceID::FALLEN_ANGEL,  // x MAGICA
        (uint8_t)RaceID::EVIL_DRAGON,   // x BIRD
        (uint8_t)RaceID::NOCTURNE       // x BEAST
    },
    // x BEAST
    {
        (uint8_t)RaceID::HOLY_BEAST,    // x GOD
        (uint8_t)RaceID::NOCTURNE,      // x AERIAL
        (uint8_t)RaceID::DEMIGOD,       // x GUARDIAN
        (uint8_t)RaceID::FALLEN_ANGEL,  // x DEMONIAC
        (uint8_t)RaceID::DEMIGOD,       // x DRAGON
        (uint8_t)RaceID::FAIRY,         // x MAGICA
        (uint8_t)RaceID::FAIRY,         // x BIRD
        (uint8_t)RaceID::EARTH_ELEMENT  // x BEAST
    }
},
// 鬼族 DEMONIAC
{
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    // x DRAGON
    {
        (uint8_t)RaceID::EARTH_MOTHER,  // x GOD
        (uint8_t)RaceID::DRAGON,        // x AERIAL
        (uint8_t)RaceID::DRAGON,        // x GUARDIAN
        (uint8_t)RaceID::VILE,          // x DEMONIAC
        (uint8_t)RaceID::FOUL,          // x DRAGON
        (uint8_t)RaceID::YOMA,          // x MAGICA
        (uint8_t)RaceID::NOCTURNE,      // x BIRD
        (uint8_t)RaceID::DRAGON_KING    // x BEAST
    },
    // x MAGICA
    {
        (uint8_t)RaceID::EARTH_MOTHER,  // x GOD
        (uint8_t)RaceID::RAPTOR,        // x AERIAL
        (uint8_t)RaceID::EVIL_DEMON,    // x GUARDIAN
        (uint8_t)RaceID::EVIL_DEMON,    // x DEMONIAC
        (uint8_t)RaceID::DIVINE,        // x DRAGON
        (uint8_t)RaceID::NOCTURNE,      // x MAGICA
        (uint8_t)RaceID::BEAST,         // x BIRD
        (uint8_t)RaceID::EARTH_ELEMENT  // x BEAST
    },
    // x BIRD
    {
        (uint8_t)RaceID::HOLY_BEAST,    // x GOD
        (uint8_t)RaceID::NOCTURNE,      // x AERIAL
        (uint8_t)RaceID::BRUTE,         // x GUARDIAN
        (uint8_t)RaceID::HAUNT,         // x DEMONIAC
        (uint8_t)RaceID::BRUTE,         // x DRAGON
        (uint8_t)RaceID::FALLEN_ANGEL,  // x MAGICA
        (uint8_t)RaceID::YOMA,          // x BIRD
        (uint8_t)RaceID::YOMA           // x BEAST
    },
    // x BEAST
    {
        (uint8_t)RaceID::DEMIGOD,       // x GOD
        (uint8_t)RaceID::BRUTE,         // x AERIAL
        (uint8_t)RaceID::DRAGON,        // x GUARDIAN
        (uint8_t)RaceID::HAUNT,         // x DEMONIAC
        (uint8_t)RaceID::FALLEN_ANGEL,  // x DRAGON
        (uint8_t)RaceID::FEMME,         // x MAGICA
        (uint8_t)RaceID::WILDER,        // x BIRD
        (uint8_t)RaceID::RAPTOR         // x BEAST
    }
},
// 龍族 DRAGON
{
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    // x MAGICA
    {
        (uint8_t)RaceID::DRAGON,        // x GOD
        (uint8_t)RaceID::WILD_BIRD,     // x AERIAL
        (uint8_t)RaceID::HEAVENLY_GOD,  // x GUARDIAN
        (uint8_t)RaceID::VILE,          // x DEMONIAC
        (uint8_t)RaceID::DRAGON,        // x DRAGON
        (uint8_t)RaceID::DIVINE,        // x MAGICA
        (uint8_t)RaceID::EVIL_DRAGON,   // x BIRD
        (uint8_t)RaceID::WILD_BIRD      // x BEAST
    },
    // x BIRD
    {
        (uint8_t)RaceID::DRAGON,        // x GOD
        (uint8_t)RaceID::AVIAN,         // x AERIAL
        (uint8_t)RaceID::BRUTE,         // x GUARDIAN
        (uint8_t)RaceID::EVIL_DRAGON,   // x DEMONIAC
        (uint8_t)RaceID::DRAGON_KING,   // x DRAGON
        (uint8_t)RaceID::BEAST,         // x MAGICA
        (uint8_t)RaceID::RAPTOR,        // x BIRD
        (uint8_t)RaceID::FAIRY          // x BEAST
    },
    // x BEAST
    {
        (uint8_t)RaceID::REAPER,        // x GOD
        (uint8_t)RaceID::RAPTOR,        // x AERIAL
        (uint8_t)RaceID::FEMME,         // x GUARDIAN
        (uint8_t)RaceID::FOUL,          // x DEMONIAC
        (uint8_t)RaceID::BRUTE,         // x DRAGON
        (uint8_t)RaceID::FALLEN_ANGEL,  // x MAGICA
        (uint8_t)RaceID::FALLEN_ANGEL,  // x BIRD
        (uint8_t)RaceID::HAUNT          // x BEAST
    }
},
// 魔族 MAGICA
{
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    // x BIRD
    {
        (uint8_t)RaceID::YOMA,          // x GOD
        (uint8_t)RaceID::HEAVENLY_GOD,  // x AERIAL
        (uint8_t)RaceID::RAPTOR,        // x GUARDIAN
        (uint8_t)RaceID::DRAGON_KING,   // x DEMONIAC
        (uint8_t)RaceID::FEMME,         // x DRAGON
        (uint8_t)RaceID::WILD_BIRD,     // x MAGICA
        (uint8_t)RaceID::DIVINE,        // x BIRD
        (uint8_t)RaceID::WILDER         // x BEAST
    },
    // x BEAST
    {
        (uint8_t)RaceID::WILD_BIRD,     // x GOD
        (uint8_t)RaceID::DRAGON_KING,   // x AERIAL
        (uint8_t)RaceID::FAIRY,         // x GUARDIAN
        (uint8_t)RaceID::BRUTE,         // x DEMONIAC
        (uint8_t)RaceID::EVIL_DEMON,    // x DRAGON
        (uint8_t)RaceID::DIVINE,        // x MAGICA
        (uint8_t)RaceID::FEMME,         // x BIRD
        (uint8_t)RaceID::WILDER         // x BEAST
    }
},
// 鳥族 BIRD
{
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    // x BEAST
    {
        (uint8_t)RaceID::GODLY_BEAST,   // x GOD
        (uint8_t)RaceID::RAPTOR,        // x AERIAL
        (uint8_t)RaceID::EARTH_ELEMENT, // x GUARDIAN
        (uint8_t)RaceID::EARTH_ELEMENT, // x DEMONIAC
        (uint8_t)RaceID::EVIL_DRAGON,   // x DRAGON
        (uint8_t)RaceID::FEMME,         // x MAGICA
        (uint8_t)RaceID::YOMA,          // x BIRD
        (uint8_t)RaceID::BEAST          // x BEAST
    }
}
};

uint8_t INHERITENCE_SKILL_MAP[21][21] =
{
{ 100, 63, 63, 63, 63, 63, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 100, 15 },   // SLASH
{ 63, 100, 63, 63, 63, 63, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 100, 15 },   // THRUST
{ 63, 63, 100, 63, 63, 63, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 100, 15 },   // STRIKE
{ 63, 63, 63, 100, 63, 63, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 100, 15 },   // LNGR
{ 63, 63, 63, 63, 100, 63, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 100, 15 },   // PIERCE
{ 63, 63, 63, 63, 63, 100, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 100, 15 },   // SPREAD
{ 31, 31, 31, 31, 31, 31, 100, 15, 45, 45, 45, 45, 45, 31, 31, 31, 31, 31, 31, 21, 100 },   // FIRE
{ 31, 31, 31, 31, 31, 31, 15, 100, 45, 45, 45, 45, 45, 31, 31, 31, 31, 31, 31, 21, 100 },   // ICE
{ 31, 31, 31, 31, 31, 31, 63, 63, 100, 45, 45, 45, 45, 31, 31, 31, 31, 31, 31, 21, 100 },   // ELCE
{ 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15 },     // ALMIGHTY
{ 31, 31, 31, 31, 31, 31, 63, 63, 45, 100, 45, 45, 45, 31, 31, 31, 31, 31, 31, 21, 100 },   // FORCE
{ 31, 31, 31, 31, 31, 31, 45, 45, 45, 45, 100, 15, 45, 31, 31, 31, 31, 31, 31, 31, 85 },    // EXPEL
{ 31, 31, 31, 31, 31, 31, 45, 45, 45, 45, 15, 100, 15, 31, 31, 31, 31, 31, 31, 31, 85 },    // CURSE
{ 15, 15, 15, 15, 15, 15, 31, 31, 31, 31, 63, 15, 100, 85, 31, 31, 31, 31, 31, 21, 31 },    // HEAL
{ 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 85, 100, 63, 63, 63, 63, 63, 63, 63 },    // SUPPORT
{ 21, 21, 21, 21, 21, 21, 31, 31, 31, 31, 31, 31, 31, 31, 100, 63, 63, 63, 63, 21, 63 },    // MAGICFORCE
{ 21, 21, 21, 21, 21, 21, 31, 31, 31, 31, 31, 31, 31, 31, 63, 100, 63, 63, 63, 21, 63 },    // NERVE
{ 21, 21, 21, 21, 21, 21, 31, 31, 31, 31, 31, 31, 31, 31, 63, 63, 100, 63, 63, 21, 63 },    // MIND
{ 21, 21, 21, 21, 21, 21, 31, 31, 31, 31, 31, 31, 31, 31, 63, 63, 63, 100, 63, 21, 63 },    // WORD
{ 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 63, 100, 63, 63 },    // SPECIAL
{ 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15 }      // SUICIDE
};