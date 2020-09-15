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

// libcomp Includes
#include <EnumUtils.h>
#include <ErrorCodes.h>

// client Includes
#include "ChannelScene.h"
#include "GameWorker.h"
#include "LoginDialog.h"

// logic Messages
#include <MessageCharacterList.h>
#include <MessageConnected.h>
#include <MessageConnectionInfo.h>
#include <MessageStartGame.h>

// packet Includes
#include <PacketLobbyCharacterList.h>
#include <PacketLobbyCharacterListEntry.h>

// Qt Includes
#include <QDateTime>
#include <QMessageBox>

using namespace game;

using libcomp::Message::MessageClientType;

LobbyScene::LobbyScene(GameWorker *pWorker, QWidget *pParent)
    : QWidget(pParent), mGameWorker(pWorker) {
  ui.setupUi(this);

  connect(ui.characterList, SIGNAL(itemSelectionChanged()), this,
          SLOT(selectionChanged()));
  connect(ui.startGame, SIGNAL(clicked(bool)), this, SLOT(startGame()));

  selectionChanged();
}

LobbyScene::~LobbyScene() {}

bool LobbyScene::ProcessClientMessage(
    const libcomp::Message::MessageClient *pMessage) {
  switch (to_underlying(pMessage->GetMessageClientType())) {
    case to_underlying(MessageClientType::CHARACTER_LIST_UPDATE):
      return HandleCharacterListUpdate(pMessage);
    case to_underlying(MessageClientType::CONNECTED_TO_CHANNEL):
      return HandleConnectedToChannel(pMessage);
    default:
      break;
  }

  return false;
}

bool LobbyScene::HandleCharacterListUpdate(
    const libcomp::Message::MessageClient *pMessage) {
  const logic::MessageCharacterList *pCharListData =
      reinterpret_cast<const logic::MessageCharacterList *>(pMessage);
  auto pPayload = pCharListData->GetPayload();
  time_t lastLoginTime = pPayload->GetLastLogin();

  ui.characterList->clear();

  for (auto pCharacter : pPayload->GetCharacters()) {
    auto characterEntry = tr("%1 (Lv %2)")
                              .arg(qs(pCharacter->GetName()))
                              .arg(pCharacter->GetLevel());

    ui.characterList->addItem(characterEntry);
  }

  ui.ticketCount->setText(QString::number(pPayload->GetTicketCount()));
  ui.characterCount->setText(tr("%1/20").arg(pPayload->CharactersCount()));
  ui.lastLogin->setText(
      tr("Last Login: %1")
          .arg(lastLoginTime ? QDateTime::fromTime_t((uint)lastLoginTime)
                                   .toString(tr("yyyy/MM/dd hh:mm"))
                             : tr("----/--/-- --:--")));

  mCharacterList = pPayload;

  selectionChanged();

  return true;
}

bool LobbyScene::HandleConnectedToChannel(
    const libcomp::Message::MessageClient *pMessage) {
  const logic::MessageConnectedToChannel *pMsg =
      reinterpret_cast<const logic::MessageConnectedToChannel *>(pMessage);

  if (ErrorCodes_t::SUCCESS == pMsg->GetErrorCode()) {
    // Show the channel.
    mGameWorker->GetChannelScene()->show();
    close();

    // Enable the dialog again.
    setEnabled(true);
  } else {
    QString errorMessage;

    // Get and display the error message.
    switch (to_underlying(pMsg->GetErrorCode())) {
      case to_underlying(ErrorCodes_t::BAD_USERNAME_PASSWORD):
        errorMessage = tr("Invalid username or password");
        break;
      case to_underlying(ErrorCodes_t::ACCOUNT_STILL_LOGGED_IN):
        errorMessage = tr("Account is still logged in");
        break;
      case to_underlying(ErrorCodes_t::SERVER_FULL):
        errorMessage = tr("Server is full");
        break;
      case to_underlying(ErrorCodes_t::WRONG_CLIENT_VERSION):
        errorMessage = tr("Please update your client");
        break;
      case to_underlying(ErrorCodes_t::CONNECTION_TIMEOUT):
        errorMessage = tr("Connection to server has timed out");
        break;
      default:
        errorMessage = tr("Unknown error");
        break;
    }

    errorMessage =
        QString("<font color=\"Red\"><b>%1</b></font>").arg(errorMessage);

    QMessageBox::critical(this, tr("Login Error"), errorMessage);
  }

  return true;
}

void LobbyScene::closeEvent(QCloseEvent *pEvent) {
  if (isEnabled()) {
    // Show the login dialog again.
    mGameWorker->SendToLogic(new logic::MessageConnectionClose());
    mGameWorker->GetLoginDialog()->show();
  }

  // Continue with the event.
  QWidget::closeEvent(pEvent);
}

void LobbyScene::selectionChanged() {
  bool haveSelection = !ui.characterList->selectedItems().isEmpty();
  bool canCreate = false;

  if (mCharacterList) {
    canCreate = (mCharacterList->GetTicketCount() ||
                 0 == mCharacterList->CharactersCount()) &&
                20 > mCharacterList->CharactersCount();
  }

  ui.startGame->setEnabled(haveSelection);
  ui.createCharacter->setEnabled(canCreate);
  ui.deleteCharacter->setEnabled(haveSelection);
}

void LobbyScene::startGame() {
  auto selectedItems = ui.characterList->selectedItems();

  if (!mCharacterList || selectedItems.isEmpty()) {
    return;
  }

  size_t idx = (size_t)ui.characterList->row(selectedItems.at(0));
  auto pCharacter = mCharacterList->GetCharacters(idx);

  // Send the start game request.
  mGameWorker->SendToLogic(
      new logic::MessageRequestStartGame(pCharacter->GetCharacterID()));

  // Disable the UI until the reply comes back.
  setEnabled(false);
}
