/**
 * @file server/channel/src/ChatManager.h
 * @ingroup channel
 *
 * @author HikaruM
 *
 * @brief Manages Chat Messages and GM Commands
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

#ifndef SERVER_CHANNEL_SRC_CHATMANAGER_H
#define SERVER_CHANNEL_SRC_CHATMANAGER_H

#include "ChannelClientConnection.h"

namespace channel
{

class ChannelServer;

/**
 * Chat type sent from and returned to the client to signify what type of
 * chat message is being sent.  This value is used to determine the
 * ChatVis_t level.
 */
enum ChatType_t : uint16_t
{

    CHAT_PARTY = 41,
    CHAT_SHOUT = 44,
    CHAT_SAY = 45,
    CHAT_TELL = 46,
    CHAT_SELF = 47,
    CHAT_CLAN = 48,
    CHAT_TEAM = 714,
};

/*
 * Visiblity for a chat message used internally to route messages. This level
 * is determined by the ChatType_t value.
 */
enum ChatVis_t : uint16_t
{
    CHAT_VIS_SELF,
    CHAT_VIS_PARTY,
    CHAT_VIS_ZONE,
    CHAT_VIS_RANGE,
    CHAT_VIS_CLAN,
    CHAT_VIS_TEAM,
};

/**
 * Manager to handle chat communication and processing of GM commands.
 */
class ChatManager
{
public:
    /**
     * Create a new ChatManager.
     * @param server Pointer back to the channel server this
     *  belongs to
     */
    ChatManager(const std::weak_ptr<ChannelServer>& server);

    /**
     * Clean up the ChatManager.
     */
    ~ChatManager();

    /**
     * Send a chat message to the specified chat channel.
     * @param client Pointer to the client that sent the message
     * @param chatChannel Channel to send the chat message to
     * @param message Message to send
     * @return true if the message was handled properly, else false
     */
    bool SendChatMessage(const std::shared_ptr<
        channel::ChannelClientConnection>& client, ChatType_t chatChannel,
        const libcomp::String& message);

    /**
     * Send a tell message to the specified character.
     * @param client Pointer to the client that sent the message
     * @param message Message to send
     * @param targetName Name of the player the message is sending to
     * @return true if the message was handled properly, else false
     */
    bool SendTellMessage(const std::shared_ptr<
        ChannelClientConnection>& client, const libcomp::String& message,
        const libcomp::String& targetName);

    /**
     * Execute a GM command using the supplied arguments.
     * @param client Pointer to the client that sent the command
     * @param cmd Command to execute
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool ExecuteGMCommand(const std::shared_ptr<
        channel::ChannelClientConnection>& client, const libcomp::String& cmd,
        const std::list<libcomp::String>& args);

private:
    /**
     * GM command to add CP to a target account.
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_AddCP(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to announce message to server .
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_Announce(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to ban a player currently on the channel
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_Ban(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to add a demon to a character's COMP.
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_Contract(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to crash the server (for testing).
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_Crash(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to apply a status effect to the client's character or
     * demon.
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_Effect(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to spawn an entity in the client's zone.
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_Enemy(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to extend a character's max expertise points available.
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_ExpertiseExtend(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to update a character's expertise as if a skill
     * were used.
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_ExpertiseUpdate(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to update the familiarity level of the client's
     * partner demon.
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_Familiarity(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to display or update a zone or instance flag.
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_Flag(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to move one player to another player.
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_Goto(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to display help for other commands.
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_Help(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to set the client character's homepoint to their
     * current zone's first spawn in point encountered.
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_Homepoint(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to create a new zone instance and enter it.
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_Instance(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to add an item to a character's inventory.
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_Item(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to kick a player from the channel
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_Kick(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to kill a character in the same zone.
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_Kill(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to level up a character or demon.
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_LevelUp(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to set a character's LNC alignment value.
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_LNC(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to set a character's obtained maps.
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_Map(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to set a character's obtained plugins.
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_Plugin(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to get a character's position.
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_Position(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to add an item to a character's post.
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_Post(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to set a specific quest to a specific phase,
     * ignoring prior requirements and state.
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_Quest(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to throw away an item in the client or target
     * character's inventory. Useful for destroying items that cannot
     * be stored or thrown away.
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_Scrap(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to have the client's character or partner demon
     * learn a skill.
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_Skill(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to have the client's character gain allocatable
     * skill points.
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_SkillPoint(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to add a mod slot to an item equipped by the client's
     * character.
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_SlotAdd(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to have the client's partner demon gain soul points.
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_SoulPoints(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to refresh all the spawn groups in the client's zone.
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_Spawn(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to update the client character or partner demon's
     * running speed.
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_Speed(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to set the default ticker message upon login
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled problerly, else false
     */
    bool GMCommand_TickerMessage(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to add a tokusei to the client's character or partner
     * demon, detached from any item, skill or status effect.
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled problerly, else false
     */
    bool GMCommand_Tokusei(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to set a character's current valuables.
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_Valuable(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to print the server version.
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_Version(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to offset the world time (of this channel only) by
     * a specified number of seconds.
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_WorldTime(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to zone to a new map.
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_Zone(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * GM command to increase the XP of a character or demon.
     * @param client Pointer to the client that sent the command
     * @param args List of arguments for the command
     * @return true if the command was handled properly, else false
     */
    bool GMCommand_XP(const std::shared_ptr<
        channel::ChannelClientConnection>& client,
        const std::list<libcomp::String>& args);

    /**
     * Determine if the client has appropriate user level necessary
     * to execute a GMand. If the client's level is too low, a message
     * will be returned informing them of the error.
     * @param client Pointer to the client that sent the command
     * @param requiredLevel User level required to execute the GMand
     * @return true if the client has the necessary user level
     */
    bool HaveUserLevel(const std::shared_ptr<
        channel::ChannelClientConnection>& client, int32_t requiredLevel);

    /**
     * Get the next argument from the supplied argument list as a string.
     * @param outVal Output variable to return the string argument to
     * @param args List of arguments read and update
     * @param encoding Optional encoding to use when reading the string.
     *  Defaults to UTF8.
     * @return true if there was an argument in the list, else false
     */
    bool GetStringArg(libcomp::String& outVal,
        std::list<libcomp::String>& args, libcomp::Convert::Encoding_t encoding
        = libcomp::Convert::Encoding_t::ENCODING_UTF8) const;

    /**
     * Get the next argument from the supplied argument list as an integer
     * type.
     * @param outVal Output variable to return the integer argument to
     * @param args List of arguments read and update
     * @return true if there was an argument in the list that was an
     *  integer, else false
     */
    template<typename T>
    bool GetIntegerArg(T& outVal, std::list<libcomp::String>& args) const
    {
        if(args.size() == 0)
        {
            return false;
        }

        bool ok = true;
        outVal = args.front().ToInteger<T>(&ok);
        if(ok)
        {
            args.pop_front();
        }

        return ok;
    }

    /**
     * Get next argument from the supplied argument list as a float/long type.
     * @param outVal Output variable to return the argument to
     * @param args List of arguments read and update
     * @return true if there was an argument in the list that was an
     *  float/long, else false
     */
    template<typename T>
    bool GetDecimalArg(T& outVal, std::list<libcomp::String>& args) const
    {
        if (args.size() == 0)
        {
            return false;
        }

        bool ok = true;
        outVal = args.front().ToDecimal<T>(&ok);
        if (ok)
        {
            args.pop_front();
        }

        return ok;
    }

    /// Pointer to the channel server
    std::weak_ptr<ChannelServer> mServer;

    /// List of GM command parsers.
    std::unordered_map<libcomp::String, std::function<bool(ChatManager&,
        const std::shared_ptr<channel::ChannelClientConnection>&,
        const std::list<libcomp::String>&)>> mGMands;
};

}
#endif
