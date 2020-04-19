/**
 * @file client/src/LobbyScene.cpp
 * @ingroup client
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Lobby scene.
 *
 * This file is part of the COMP_hack Test Client (client).
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

#include "LobbyScene.h"

// utility Includes
#include <time.h>

// libcomp Includes
#include <EnumUtils.h>
#include <ErrorCodes.h>

// client Includes
#include "GameWorker.h"
#include "LoginDialog.h"

// logic Messages
#include <MessageConnectionInfo.h>
#include <MessageCharacterList.h>

// packet Includes
#include <PacketLobbyCharacterList.h>
#include <PacketLobbyCharacterListEntry.h>

using namespace game;

using libcomp::Message::MessageClientType;

LobbyScene::LobbyScene(GameWorker *pWorker, QWidget *pParent) :
    QWidget(pParent), mGameWorker(pWorker)
{
    ui.setupUi(this);
}

LobbyScene::~LobbyScene()
{
}

bool LobbyScene::ProcessClientMessage(
    const libcomp::Message::MessageClient *pMessage)
{
    switch(to_underlying(pMessage->GetMessageClientType()))
    {
        case to_underlying(MessageClientType::CHARACTER_LIST_UPDATE):
            return HandleCharacterListUpdate(pMessage);
        default:
            break;
    }

    return false;
}
bool LobbyScene::HandleCharacterListUpdate(const libcomp::Message::MessageClient *pMessage)
{   
    const logic::MessageCharacterList *pCharListData =
        reinterpret_cast<const logic::MessageCharacterList *>(pMessage);
    auto characterPayload = pCharListData->GetPayload();
    auto characterList = characterPayload->GetCharacters();
    time_t lastLoginTime = characterPayload->GetLastLogin();
    for (auto character : characterList) {
        QString characterEntry;
        QString characterName = QString::fromStdString(character->GetName().ToUtf8());
        QString characterLevel = QString::number(character->GetLevel());
        characterEntry.append(characterName);
        characterEntry.append(QString::fromStdString(" Lv "));
        characterEntry.append(characterLevel);
        ui.characterList->addItem(characterEntry);
    }
    QString characterCountStr = QString::number(characterPayload->CharactersCount());
    characterCountStr.append(QString::fromStdString("/20"));
    QString lastLoginStr = QString::fromStdString("Last Login: ");
    lastLoginStr.append(!lastLoginTime ? QString::fromStdString("----/ --/ -- --:--") : ctime(&lastLoginTime));
    
    ui.ticketCount->setText(QString::number(characterPayload->GetTicketCount()));
    ui.characterCount->setText(characterCountStr);
    ui.lastLogin->setText(lastLoginStr);
    
    return true;
}

void LobbyScene::closeEvent(QCloseEvent *pEvent)
{
    // Show the login dialog again.
    mGameWorker->SendToLogic(new logic::MessageConnectionClose());
    mGameWorker->GetLoginDialog()->show();

    // Continue with the event.
    QWidget::closeEvent(pEvent);
}
