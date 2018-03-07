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

#include "LoginHandlerThread.h"

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

// object Includes
#include <LoginScriptRequest.h>
#include <LoginScriptReply.h>

namespace libcomp
{

template<>
ScriptEngine& ScriptEngine::Using<objects::LoginScriptRequest>()
{
    if(!BindingExists("LoginScriptRequest"))
    {
        Sqrat::Enumeration op(mVM);
        op.Const("OPERATION_GET", to_underlying(
            objects::LoginScriptRequest::OperationType_t::GET));
        op.Const("OPERATION_LOGIN", to_underlying(
            objects::LoginScriptRequest::OperationType_t::LOGIN));
        op.Const("OPERATION_QUIT", to_underlying(
            objects::LoginScriptRequest::OperationType_t::QUIT));

        Sqrat::ConstTable(mVM).Enum("Operation_t", op);

        Sqrat::Class<objects::LoginScriptRequest> binding(mVM,
            "LoginScriptRequest");
        binding
            .Prop("operation", &objects::LoginScriptRequest::GetOperation,
                &objects::LoginScriptRequest::SetOperation)
            .Prop("page", &objects::LoginScriptRequest::GetPage,
                &objects::LoginScriptRequest::SetPage)
            .Prop("pageError", &objects::LoginScriptRequest::GetPageError,
                &objects::LoginScriptRequest::SetPageError)
            .Prop("username", &objects::LoginScriptRequest::GetUsername,
                &objects::LoginScriptRequest::SetUsername)
            .Prop("password", &objects::LoginScriptRequest::GetPassword,
                &objects::LoginScriptRequest::SetPassword)
            .Prop("clientVersion",
                &objects::LoginScriptRequest::GetClientVersion,
                &objects::LoginScriptRequest::SetClientVersion)
            .Prop("rememberUsername",
                &objects::LoginScriptRequest::GetRememberUsername,
                &objects::LoginScriptRequest::SetRememberUsername)
            .Func("postVarExists",
                &objects::LoginScriptRequest::PostVarsKeyExists)
            .Func<libcomp::String (objects::LoginScriptRequest::*)(
                const libcomp::String&)>("postVar",
                &objects::LoginScriptRequest::GetPostVars)
            .Func("postVarCount",
                &objects::LoginScriptRequest::PostVarsCount)
            .Func<bool (objects::LoginScriptRequest::*)(
                const libcomp::String&, const libcomp::String&)>(
                "postVarSet", &objects::LoginScriptRequest::SetPostVars)
            .Func("postVarRemove",
                &objects::LoginScriptRequest::RemovePostVars)
            ; // Last call to binding
        Bind<objects::LoginScriptRequest>("LoginScriptRequest", binding);
    }

    return *this;
}

template<>
ScriptEngine& ScriptEngine::Using<objects::LoginScriptReply>()
{
    if(!BindingExists("LoginScriptReply"))
    {
        Sqrat::Class<objects::LoginScriptReply> binding(mVM,
            "LoginScriptReply");
        binding
            .Prop("username", &objects::LoginScriptReply::GetUsername,
                &objects::LoginScriptReply::SetUsername)
            .Prop("password", &objects::LoginScriptReply::GetPassword,
                &objects::LoginScriptReply::SetPassword)
            .Prop("clientVersion",
                &objects::LoginScriptReply::GetClientVersion,
                &objects::LoginScriptReply::SetClientVersion)
            .Prop("rememberUsername",
                &objects::LoginScriptReply::GetRememberUsername,
                &objects::LoginScriptReply::SetRememberUsername)
            .Func("replaceVarExists",
                &objects::LoginScriptReply::ReplaceVarsKeyExists)
            .Func<libcomp::String (objects::LoginScriptReply::*)(
                const libcomp::String&)>("replaceVar",
                &objects::LoginScriptReply::GetReplaceVars)
            .Func("replaceVarCount",
                &objects::LoginScriptReply::ReplaceVarsCount)
            .Func<bool (objects::LoginScriptReply::*)(const libcomp::String&,
                const libcomp::String&)>("replaceVarSet",
                &objects::LoginScriptReply::SetReplaceVars)
            .Func("replaceVarRemove",
                &objects::LoginScriptReply::RemoveReplaceVars)
            .Prop("loginOK", &objects::LoginScriptReply::GetLoginOK,
                &objects::LoginScriptReply::SetLoginOK)
            .Prop("lockControls", &objects::LoginScriptReply::GetLockControls,
                &objects::LoginScriptReply::SetLockControls)
            .Prop("errorMessage", &objects::LoginScriptReply::GetErrorMessage,
                &objects::LoginScriptReply::SetErrorMessage)
            .Prop("sid1", &objects::LoginScriptReply::GetSID1,
                &objects::LoginScriptReply::SetSID1)
            .Prop("sid2", &objects::LoginScriptReply::GetSID2,
                &objects::LoginScriptReply::SetSID2)
            ; // Last call to binding
        Bind<objects::LoginScriptReply>("LoginScriptReply", binding);
    }

    return *this;
}

} // namespace libcomp

using namespace lobby;

LoginHandlerThread::LoginHandlerThread() : mDidInit(false)
{
    mEngine.Using<objects::LoginScriptRequest>();
    mEngine.Using<objects::LoginScriptReply>();
}

LoginHandlerThread::~LoginHandlerThread()
{
}

bool LoginHandlerThread::DidInit() const
{
    return mDidInit;
}

bool LoginHandlerThread::Init(const libcomp::String& script)
{
    if(!mDidInit)
    {
        mDidInit = true;

        return mEngine.Eval(script);
    }

    return true;
}

bool LoginHandlerThread::ProcessLoginRequest(const std::shared_ptr<
        objects::LoginScriptRequest>& req)
{
    req->SetOperation(to_underlying(
        objects::LoginScriptRequest::OperationType_t::ERROR));

    auto ref = Sqrat::RootTable(mEngine.GetVM()).GetFunction(
        "ProcessLoginRequest").Evaluate<bool>(req);

    if(!ref || !(*ref))
    {
        LOG_ERROR("Failed to process login request.\n");

        return false;
    }

    return true;
}

bool LoginHandlerThread::ProcessLoginReply(const std::shared_ptr<
        objects::LoginScriptReply>& reply)
{
    auto ref = Sqrat::RootTable(mEngine.GetVM()).GetFunction(
        "ProcessLoginReply").Evaluate<bool>(reply);

    if(!ref || !(*ref))
    {
        LOG_ERROR("Failed to process login reply.\n");

        return false;
    }

    return true;
}
