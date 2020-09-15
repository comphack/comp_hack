/**
 * @file client/src/ChannelScene.cpp
 * @ingroup client
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Channel scene.
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

#include "ChannelScene.h"

// libcomp Includes
#include <EnumUtils.h>
#include <ErrorCodes.h>

// client Includes
#include "GameWorker.h"
#include "LoginDialog.h"

// logic Messages
#include <MessageAccountDump.h>
#include <MessageConnectionInfo.h>

// packet Includes
#include <PacketLobbyCharacterList.h>
#include <PacketLobbyCharacterListEntry.h>

// Qt Includes
#include <QFileDialog>
#include <QMessageBox>

using namespace game;

using libcomp::Message::MessageClientType;

ChannelScene::ChannelScene(GameWorker *pWorker, QWidget *pParent)
    : QWidget(pParent), mGameWorker(pWorker) {
  ui.setupUi(this);

  connect(ui.backupAccount, SIGNAL(clicked(bool)), this, SLOT(backupAccount()));
}

ChannelScene::~ChannelScene() {}

bool ChannelScene::ProcessClientMessage(
    const libcomp::Message::MessageClient *pMessage) {
  switch (to_underlying(pMessage->GetMessageClientType())) {
    case to_underlying(MessageClientType::ACCOUNT_DUMP_STATUS):
      return HandleAccountDumpStatus(pMessage);
    default:
      break;
  }

  return false;
}

bool ChannelScene::HandleAccountDumpStatus(
    const libcomp::Message::MessageClient *pMessage) {
  const logic::MessageAccountDumpStatus *pStatus =
      reinterpret_cast<const logic::MessageAccountDumpStatus *>(pMessage);

  if (pStatus->IsSuccess()) {
    QMessageBox::information(this, tr("Backup Account"),
                             tr("Account has been backed up!"));
  } else {
    QMessageBox::critical(this, tr("Backup Account"),
                          tr("Account backup has failed!"));
  }

  ui.backupAccount->setEnabled(true);

  return true;
}

void ChannelScene::closeEvent(QCloseEvent *pEvent) {
  // Show the login dialog again.
  mGameWorker->SendToLogic(new logic::MessageConnectionClose());
  mGameWorker->GetLoginDialog()->show();

  // Continue with the event.
  QWidget::closeEvent(pEvent);
}

void ChannelScene::backupAccount() {
  // Prompt the user for the save dialog.
  auto path = QFileDialog::getSaveFileName(
      this, tr("Backup Account"), QString(), tr("Account Data (*.xml)"));

  if (path.isEmpty()) {
    return;
  }

  // Send the account dump request.
  mGameWorker->SendToLogic(new logic::MessageAccountDump(cs(path)));

  ui.backupAccount->setEnabled(false);
}
