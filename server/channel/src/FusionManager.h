/**
 * @file server/channel/src/FusionManager.h
 * @ingroup channel
 *
 * @author HACKfrost
 *
 * @brief Manager class used to handle all demon fusion based actions.
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

#ifndef SERVER_CHANNEL_SRC_FUSIONMANAGER_H
#define SERVER_CHANNEL_SRC_FUSIONMANAGER_H

// channel Includes
#include "ChannelClientConnection.h"

namespace objects
{
class Demon;
class MiDevilData;
}

namespace channel
{

class ChannelServer;

/**
 * Manager class used to handle all demon fusion based actions.
 */
class FusionManager
{
public:
    /**
     * Create a new FusionManager
     * @param server Pointer back to the channel server this belongs to.
     */
    FusionManager(const std::weak_ptr<ChannelServer>& server);

    /**
     * Clean up the FusionManager
     */
    virtual ~FusionManager();

    /**
     * Perform a normal 2-way fusion and respond to the client with the
     * results
     * @param client Pointer to the client performing the fusion
     * @param demonID1 ID of the first demon being fused
     * @param demonID2 ID of the second demon being fused
     * @param costItemType Optional cost item type used to pay for the fusion,
     *  defaults to macca
     * @return true if the fusion succeeded, false if it did not
     */
    bool HandleFusion(const std::shared_ptr<
        ChannelClientConnection>& client, int64_t demonID1, int64_t demonID2,
        uint32_t costItemType = 0);

    /**
     * Perform a tri-fusion and respond to the client with the results
     * @param client Pointer to the client performing the fusion
     * @param demonID1 ID of the first demon being fused
     * @param demonID2 ID of the second demon being fused
     * @param demonID3 ID of the third demon being fused
     * @param soloFusion true if a solo fusion is being performed, false
     *  if it is a normal party based tri-fusion
     * @return true if the fusion succeeded, false if it did not
     */
    bool HandleTriFusion(const std::shared_ptr<
        ChannelClientConnection>& client, int64_t demonID1, int64_t demonID2,
        int64_t demonID3, bool soloFusion);

    /**
     * Calculate the resulting demon based upon the supplied demon IDs
     * @param client Pointer to the client performing the fusion
     * @param demonID1 ID of the first demon being fused
     * @param demonID2 ID of the second demon being fused
     * @param demonID3 ID of the third demon being fused
     * @return Type ID of the demon that would be fused
     */
    uint32_t GetResultDemon(const std::shared_ptr<
        ChannelClientConnection>& client, int64_t demonID1, int64_t demonID2,
        int64_t demonID3);

    /**
     * End any fusion based exchanges the player is a part of. If they
     * are hosting a tri-fusion, all guests will be informed as well.
     * @param client Pointer to the client in the exchange
     */
    void EndExchange(const std::shared_ptr<
        channel::ChannelClientConnection>& client);

    /**
     * Get the mitama index of the supplied type that matches the
     * FusionTables entries
     * @param mitamaType Demon type to calculate
     * @param found Output parameter indicating if the index was found
     * @return Mitama index for the supplied type
     */
    size_t GetMitamaIndex(uint32_t mitamaType, bool& found);

    /**
     * Determine if the supplied demon is valid to use for tri-fusion
     * @param demon Pointer to the demon to check
     * @return true if they can be used for tri-fusion, false if they cannot
     */
    bool IsTriFusionValid(const std::shared_ptr<objects::Demon>& demon);

private:
    /**
     * Perform a two-way or tri-fusion based upon the supplied demon IDs
     * @param client Pointer to the client performing the fusion
     * @param demonID1 ID of the first demon being fused
     * @param demonID2 ID of the second demon being fused
     * @param demonID3 ID of the third demon being fused
     * @param costItemType Optional cost item type used to pay for the fusion,
     *  defaults to macca
     * @param resultDemon Output parameter to return the resulting demon in
     * @return Error code associated to the reason why any failure may have
     *  ocurred:
     *  1 = Normal failure
     *  0 = No failure
     *  -1 = Generic/criteria error
     *  -2 = Calculated fusion failed
     *  -3 = Supplied cost type is not valid
     *  -4 = Cost could not be paid
     */
    int8_t ProcessFusion(const std::shared_ptr<
        ChannelClientConnection>& client, int64_t demonID1, int64_t demonID2,
        int64_t demonID3, uint32_t costItemType,
        std::shared_ptr<objects::Demon>& resultDemon);

    /**
     * Sum up and average the demon levels supplied and optionally
     * offset with a final adjustment value
     * @param level1 Current level of the first demon
     * @param level2 Current level of the second demon
     * @param finalLevelAdjust Optional adjustment added at the
     *  end of the calculation
     * @return Calculated level
     */
    int8_t GetAdjustedLevelSum(uint8_t level1, uint8_t level2,
        int8_t finalLevelAdjust = 0);

    /**
     * Get the resulting demon of an adjusted race level range
     * @param race Race of the demon to retrieve
     * @param adjustedLevelSum Level to use when checking level ranges
     * @return Pointer to the definition of the result demon
     */
    std::shared_ptr<objects::MiDevilData> GetResultDemon(uint8_t race,
        int8_t adjustedLevelSum);

    /**
     * Get the demon type ID associated to an elemental index from the manager
     * @param elementalIndex Elemental index to retrieve
     * @return Type ID of the result demon
     */
    uint32_t GetElementalType(size_t elementalIndex) const;

    /**
     * Get the demon type ID associated to a mitama index from the manager
     * @param mitamaIndex Mitama index to retrieve
     * @return Type ID of the result demon
     */
    uint32_t GetMitamaType(size_t mitamaIndex) const;

    /**
     * Get the demon type ID associated to an elemental to non-elemental fusion
     * @param elementalType Elemental demon type
     * @param otherRace Race of the non-elemental demon
     * @param otherType Type of the non-elemental demon
     * @return Type ID of the result demon
     */
    uint32_t GetElementalFuseResult(uint32_t elementalType, uint8_t otherRace,
        uint32_t otherType);

    /**
     * Get the race index of the supplied race that matches the FusionTables
     * entries
     * @param otherRace Race to calculate
     * @param found Output parameter indicating if the index was found
     * @return Race index for the supplied race
     */
    size_t GetRaceIndex(uint8_t raceID, bool& found);

    /**
     * Get the elemental index of the supplied type that matches the
     * FusionTables entries
     * @param elemType Demon type to calculate
     * @param found Output parameter indicating if the index was found
     * @return Elemental index for the supplied type
     */
    size_t GetElementalIndex(uint32_t elemType, bool& found);

    /**
     * Determine the type of the demon directly above or directly below
     * the supplied type in the fusion ranges by one rank
     * @param raceID Race of the demon to adjust
     * @param demonType Type of the demon to adjust
     * @param up true if checking higher, false if checking lower
     * @return Demon type directly above or below the supplied type
     */
    uint32_t RankUpDown(uint8_t raceID, uint32_t demonType, bool up);

    /// Pointer to the channel server.
    std::weak_ptr<ChannelServer> mServer;
};

} // namespace channel

#endif // SERVER_CHANNEL_SRC_FUSIONMANAGER_H
