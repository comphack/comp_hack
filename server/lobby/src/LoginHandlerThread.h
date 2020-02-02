/**
 * @file server/lobby/src/LoginHandlerThread.h
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Script interface for the login webpage.
 *
 * This file is part of the Lobby Server (lobby).
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

#ifndef SERVER_LOBBY_SRC_LOGINHANDLERTHREAD_H
#define SERVER_LOBBY_SRC_LOGINHANDLERTHREAD_H

// libcomp Includes
#include <ScriptEngine.h>
#include <CString.h>

// Standard C++11 Includes
#include <memory>

namespace objects
{

class LoginScriptRequest;
class LoginScriptReply;

} // namespace objects

namespace lobby
{


class LoginHandlerThread
{
public:
    LoginHandlerThread();
    ~LoginHandlerThread();

    bool DidInit() const;
    bool Init(const libcomp::String& script);

    bool ProcessLoginRequest(const std::shared_ptr<
        objects::LoginScriptRequest>& req);
    bool ProcessLoginReply(const std::shared_ptr<
        objects::LoginScriptReply>& reply);

private:
    bool mDidInit;

    libcomp::ScriptEngine mEngine;
};

} // namespace lobby

#endif // SERVER_LOBBY_SRC_LOGINHANDLERTHREAD_H
