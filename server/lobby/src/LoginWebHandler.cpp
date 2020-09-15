/**
 * @file server/lobby/src/LoginWebHandler.cpp
 * @ingroup lobby
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Civet login webpage handler.
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

#include "LoginWebHandler.h"

// lobby Includes
#include "AccountManager.h"
#include "ResourceLogin.h"

// libcomp Includes
#include <Log.h>
#include <PacketCodes.h>

// object Includes
#include <LoginScriptReply.h>
#include <LoginScriptRequest.h>

// libobjgen Includes
#include <Account.h>

using namespace lobby;

thread_local LoginHandlerThread LoginHandler::mThreadHandler;

LoginHandler::LoginHandler(const std::shared_ptr<libcomp::Database> &database)
    : mDatabase(database), mAccountManager(nullptr) {
  mVfs.AddArchiveLoader(new ttvfs::VFSZipArchiveLoader);

  ttvfs::CountedPtr<ttvfs::MemFile> pMemoryFile = new ttvfs::MemFile(
      "login.zip", ResourceLogin, static_cast<uint32_t>(ResourceLoginSize));

  if (!mVfs.AddArchive(pMemoryFile, "")) {
    LogWebAPICriticalMsg("Failed to add login resource archive.\n");
  }
}

LoginHandler::~LoginHandler() {}

void LoginHandler::SetConfig(
    const std::shared_ptr<objects::LobbyConfig> &config) {
  mConfig = config;

  if (!mConfig->GetWebRoot().IsEmpty()) {
    mVfs.AddVFSDir(
        new ttvfs::DiskDir(mConfig->GetWebRoot().C(), new ttvfs::DiskLoader),
        "");
  }
}

bool LoginHandler::handleGet(CivetServer *pServer,
                             struct mg_connection *pConnection) {
  auto req = std::make_shared<objects::LoginScriptRequest>();

  return HandlePage(pServer, pConnection, req);
}

bool LoginHandler::handlePost(CivetServer *pServer,
                              struct mg_connection *pConnection) {
  auto req = ParsePost(pServer, pConnection);

  if (!req) {
    req = std::make_shared<objects::LoginScriptRequest>();
  }

  return HandlePage(pServer, pConnection, req);
}

std::shared_ptr<objects::LoginScriptRequest> LoginHandler::ParsePost(
    CivetServer *pServer, struct mg_connection *pConnection) {
  (void)pServer;  // Not used.

  const mg_request_info *pRequestInfo = mg_get_request_info(pConnection);

  // Sanity check the request info.
  if (nullptr == pRequestInfo || nullptr == mConfig) {
    return {};
  }

  size_t postContentLength = static_cast<size_t>(pRequestInfo->content_length);

  // Sanity check the post content length.
  if (0 == postContentLength) {
    return {};
  }

  // Allocate.
  char *szPostData = new char[postContentLength + 1];

  // Read the post data.
  postContentLength =
      static_cast<size_t>(mg_read(pConnection, szPostData, postContentLength));
  szPostData[postContentLength] = 0;

  // Split the values.
  auto req = std::make_shared<objects::LoginScriptRequest>();
  auto valuePairs = libcomp::String(szPostData).Split("&");

  for (auto valuePair : valuePairs) {
    auto pair = valuePair.Split("=");

    if (2 == pair.size()) {
      auto it = pair.begin();
      auto key = *(it++);
      auto value = *it;

      req->SetPostVars(key, value);
    }
  }

  // Free.
  delete[] szPostData;

  return req;
}

bool LoginHandler::HandlePage(
    CivetServer *pServer, struct mg_connection *pConnection,
    const std::shared_ptr<objects::LoginScriptRequest> &req) {
  (void)pServer;

  libcomp::String uri("index.nut");

  const mg_request_info *pRequestInfo = mg_get_request_info(pConnection);

  // Sanity check the request info.
  if (nullptr == pRequestInfo) {
    return false;
  }

  // Resolve a "/" URI into "/index.nut".
  if (nullptr != pRequestInfo->local_uri &&
      libcomp::String("/") != pRequestInfo->local_uri) {
    uri = pRequestInfo->local_uri;
  }

  // Remove any leading slash.
  if ("/" == uri.Left(1)) {
    uri = uri.Mid(1);
  }

  /// Load the Squirrel handler script once per thread.
  if (!mThreadHandler.DidInit()) {
    // Attempt to load the script file.
    std::vector<char> pageData = LoadVfsFile("handler.nut");

    // This should always load but check anyway.
    if (pageData.empty() ||
        !mThreadHandler.Init(libcomp::String(&pageData[0]))) {
      LogWebAPIErrorMsg("Failed to load web script handler.nut\n");

      return false;
    }
  }

  // This session ID is never used. If you notice it being used file a bug.
  static const libcomp::String sid2 =
      "deadc0dedeadc0dedeadc0dedeadc0dedeadc0dedeadc0dedeadc0dedeadc0dedeadc0de"
      "deadc0dedeadc0dedeadc0dedeadc0dedeadc0dedeadc0dedeadc0dedeadc0dedeadc0de"
      "deadc0dedeadc0dedeadc0dedeadc0dedeadc0dedeadc0dedeadc0dedeadc0dedeadc0de"
      "deadc0dedeadc0dedeadc0dedeadc0dedeadc0dedeadc0dedeadc0dedeadc0dedeadc0de"
      "deadc0dedead";

  libcomp::String sid1;
  libcomp::String errorMessage;

  bool loginOK = true;
  bool lockControls = false;

  // Do not allow access to a handler script.
  if (uri.Contains("handler.nut")) {
    return false;
  }

  if (".nut" == uri.Right(strlen(".nut"))) {
    if (!mThreadHandler.ProcessLoginRequest(req)) {
      return false;
    }

    switch (req->GetOperation()) {
      case to_underlying(objects::LoginScriptRequest::OperationType_t::GET): {
        // Get the required client version.
        uint32_t requiredClientVersion =
            static_cast<uint32_t>(mConfig->GetClientVersion() * 1000.0f + 0.5f);

        // Get the actual client version.
        uint32_t clientVersion =
            (uint32_t)(req->GetClientVersion() * 1000.0f + 0.5f);

        // Check the client version even if this is not a POST so they
        // know before login and we can deny them more login attempts
        // by blocking the input fields on the form.
        if (requiredClientVersion != clientVersion) {
          lockControls = true;
          errorMessage = ErrorCodeString(ErrorCodes_t::WRONG_CLIENT_VERSION);
        }

        break;
      }
      case to_underlying(objects::LoginScriptRequest::OperationType_t::LOGIN): {
        // Attempt to login for the user.
        ErrorCodes_t error = mAccountManager->WebAuthLogin(
            req->GetUsername(), req->GetPassword(),
            (uint32_t)(req->GetClientVersion() * 1000.0f + 0.5f), sid1);

        if (ErrorCodes_t::SUCCESS != error) {
          loginOK = false;
          errorMessage = ErrorCodeString(error);

          // Lock the controls if the client version is wrong.
          if (ErrorCodes_t::WRONG_CLIENT_VERSION == error) {
            lockControls = true;
          }
        }

        break;
      }
      case to_underlying(objects::LoginScriptRequest::OperationType_t::QUIT): {
        // Do nothing special with a QUIT.
        break;
      }
      default:  // objects::LoginScriptRequest::OperationType_t::ERROR
      {
        loginOK = false;
        errorMessage = ErrorCodeString(ErrorCodes_t::SYSTEM_ERROR);
        break;
      }
    }

    uri = loginOK ? req->GetPage() : req->GetPageError();
  }

  LogWebAPIDebug([&]() { return libcomp::String("URI: %1\n").Arg(uri); });

  // Attempt to load the URI.
  std::vector<char> pageData = LoadVfsFile(uri);

  // Make sure the page was loaded or return a 404.
  if (pageData.empty()) {
    return false;
  }

  if (".png" == uri.Right(strlen(".png"))) {
    mg_printf(pConnection,
              "HTTP/1.1 200 OK\r\n"
              "Content-Type: image/png; charset=UTF-8\r\n"
              "Content-Length: %u\r\n"
              "Connection: close\r\n"
              "\r\n",
              (unsigned int)pageData.size());
    mg_write(pConnection, &pageData[0], pageData.size());
  } else if (".swf" == uri.Right(strlen(".swf"))) {
    mg_printf(pConnection,
              "HTTP/1.1 200 OK\r\n"
              "Content-Type: application/x-shockwave-flash\r\n"
              "Content-Length: %u\r\n"
              "Connection: close\r\n"
              "\r\n",
              (unsigned int)pageData.size());
    mg_write(pConnection, &pageData[0], pageData.size());
  } else if (".css" == uri.Right(strlen(".css"))) {
    mg_printf(pConnection,
              "HTTP/1.1 200 OK\r\n"
              "Content-Type: text/css\r\n"
              "Content-Length: %u\r\n"
              "Connection: close\r\n"
              "\r\n",
              (unsigned int)pageData.size());
    mg_write(pConnection, &pageData[0], pageData.size());
  } else  // nut or html
  {
    pageData.push_back(0);
    libcomp::String page(&pageData[0]);

    if (".nut" == uri.Right(strlen(".nut"))) {
      if (errorMessage.IsEmpty()) {
        errorMessage = "Please enter your username and password.";
      }

      auto reply = std::make_shared<objects::LoginScriptReply>();
      reply->SetUsername(req->GetUsername());
      reply->SetPassword(req->GetPassword());
      reply->SetClientVersion(req->GetClientVersion());
      reply->SetRememberUsername(req->GetRememberUsername());
      reply->SetLoginOK(loginOK);
      reply->SetLockControls(lockControls);
      reply->SetErrorMessage(errorMessage);
      reply->SetSID1(sid1);
      reply->SetSID2(sid2);

      if (!mThreadHandler.ProcessLoginReply(reply)) {
        return false;
      }

      for (auto it = reply->ReplaceVarsBegin(); it != reply->ReplaceVarsEnd();
           ++it) {
        auto key = it->first;
        auto value = it->second;

        page = page.Replace(key, value);
      }
    }

    mg_printf(pConnection,
              "HTTP/1.1 200 OK\r\n"
              "Content-Type: text/html; charset=UTF-8\r\n"
              "Content-Length: %u\r\n"
              "Connection: close\r\n"
              "\r\n",
              (unsigned int)page.Size());
    mg_write(pConnection, page.C(), page.Size());
  }

  return true;
}

std::vector<char> LoginHandler::LoadVfsFile(const libcomp::String &path) {
  std::vector<char> data;

  ttvfs::File *vf = mVfs.GetFile(path.C());
  if (!vf) {
    LogWebAPIError([&]() {
      return libcomp::String("Failed to find file: %1\n").Arg(path);
    });

    return std::vector<char>();
  }

  size_t fileSize = static_cast<size_t>(vf->size());

  data.resize(fileSize);

  if (!vf->open("rb")) {
    LogWebAPIError([&]() {
      return libcomp::String("Failed to open file: %1\n").Arg(path);
    });

    return std::vector<char>();
  }

  if (fileSize != vf->read(&data[0], fileSize)) {
    LogWebAPIError([&]() {
      return libcomp::String("Failed to read file: %1\n").Arg(path);
    });

    return std::vector<char>();
  }

  // Make sure it ends with a null terminator
  data.push_back(0);

  return data;
}

void LoginHandler::SetAccountManager(AccountManager *pManager) {
  mAccountManager = pManager;
}
