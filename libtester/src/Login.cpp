/**
 * @file libtester/src/Login.cpp
 * @ingroup libtester
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Test functions to aid in login.
 *
 * This file is part of the COMP_hack Tester Library (libtester).
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

#include "Login.h"

// libcomp Includes
#include <MessagePacket.h>
#include <Packet.h>

// Standard C++11 Includes
#include <regex>
#include <thread>

using namespace libtester;

bool Login::WebLogin(const libcomp::String& username,
                     const libcomp::String& password,
                     const libcomp::String& clientVersion,
                     libcomp::String& sid1, libcomp::String& sid2) {
  bool result = false;

  asio::io_service service;

  libcomp::String httpRequest =
      libcomp::String(
          "POST /index.nut HTTP/1.1\r\n"
          "Accept: image/gif, image/jpeg, image/pjpeg, "
          "application/x-ms-application, application/xaml+xml, "
          "application/x-ms-xbap, */*\r\n"
          "Referer: http://127.0.0.1:10999/\r\n"
          "Accept-Language: en-US\r\n"
          "Content-Type: application/x-www-form-urlencoded\r\n"
          "Accept-Encoding: gzip, deflate\r\n"
          "User-Agent: imagilla/1.0\r\n"
          "Host: 127.0.0.1:10999\r\n"
          "Content-Length: %1\r\n"
          "Connection: Keep-Alive\r\n"
          "Cache-Control: no-cache\r\n"
          "\r\n"
          "login=&ID=%2&PASS=%3&IDSAVE=on&cv=%4")
          .Arg(30 + username.Length() + password.Length() +
               clientVersion.Length())
          .Arg(username)
          .Arg(password)
          .Arg(clientVersion);

  static const std::regex replyRegEx(
      "^HTTP\\/1.1 200 OK\\r\\n"
      "Content-Type: text\\/html; charset=UTF-8\\r\\n"
      "Content-Length: ([0-9]+)\\r\\nConnection: close\\r\\n\\r\\n"
      "<html><head><meta http-equiv=\"content-type\" "
      "content=\"text\\/html; charset=UTF-8\"><\\/head><body>"
      "login...<!-- ID:\"([^\"]+)\" 1stSID:\"([a-f0-9]{300})\" "
      "2ndSID:\"([a-f0-9]{300})\" isIdSave:\"([01])\" "
      "existBirthday:\"([01])\" --><\\/body><\\/html>\n$");

  std::shared_ptr<libtester::HttpConnection> connection(
      new libtester::HttpConnection(service));

  std::shared_ptr<libcomp::MessageQueue<libcomp::Message::Message*>>
      messageQueue(new libcomp::MessageQueue<libcomp::Message::Message*>());

  connection->SetMessageQueue(messageQueue);
  connection->Connect("127.0.0.1", 10999);

  std::thread serviceThread([&service]() { service.run(); });

  asio::steady_timer timer(service);
  timer.expires_from_now(std::chrono::seconds(10));
  timer.async_wait([&service](asio::error_code) { service.stop(); });

  libcomp::Packet p;
  p.WriteArray(httpRequest.C(), static_cast<uint32_t>(httpRequest.Size()));

  connection->RequestPacket(9999);  // Over 9000!
  connection->SendPacket(p);

  std::thread worker(
      [&](std::shared_ptr<libcomp::MessageQueue<libcomp::Message::Message*>>
              queue) {
        bool haveReply = false;

        while (!haveReply) {
          std::list<libcomp::Message::Message*> msgs;

          queue->DequeueAll(msgs);

          for (auto msg : msgs) {
            libcomp::Message::Packet* pMessage =
                dynamic_cast<libcomp::Message::Packet*>(msg);

            if (nullptr != pMessage) {
              libcomp::ReadOnlyPacket reply(pMessage->GetPacket());
              std::smatch match;

              std::string source(&reply.ReadArray(reply.Size())[0],
                                 reply.Size());

              haveReply = true;

              if (std::regex_match(source, match, replyRegEx)) {
                libcomp::String contentLength = match.str(1);
                libcomp::String replyUsername = match.str(2);

                sid1 = match.str(3);
                sid2 = match.str(4);

                if ("788" == contentLength && username == replyUsername) {
                  result = true;
                }
              }

              connection.reset();
              service.stop();
            }

            delete msg;
          }
        }
      },
      messageQueue);

  serviceThread.join();
  worker.join();

  return result;
}
