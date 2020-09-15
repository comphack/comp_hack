/**
 * @file tools/logger/src/MainWindow.cpp
 * @ingroup logger
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Main window of the packet logging application.
 *
 * Copyright (C) 2010-2020 COMP_hack Team <compomega@tutanota.com>
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

#include "MainWindow.h"

#include "Close.h"
#include "LoggerServer.h"
#include "Settings.h"

// libcomp Includes
#include <Crypto.h>
#include <PEFile.h>

// Ignore warnings
#include <PushIgnore.h>

// Qt Includes
#include <QApplication>
#include <QDir>
#include <QMessageBox>
#include <QProcess>
#include <QRegExp>
#include <QSettings>
#include <QTcpSocket>
#include <QTimer>

#include "ui_About.h"

// Stop ignoring includes
#include <PopIgnore.h>

MainWindow::MainWindow(const QString &capturePath, QWidget *p)
    : QMainWindow(p), mCapturePath(capturePath) {
  ui.setupUi(this);

  // Connect the menu actions.
  connect(ui.actionShutdown, SIGNAL(triggered(bool)), this, SLOT(shutdown()));
  connect(ui.action_Captures, SIGNAL(triggered(bool)), this,
          SLOT(showCaptures()));
  connect(ui.action_Settings, SIGNAL(triggered(bool)), this,
          SLOT(showSettings()));
  connect(ui.actionAdd_client, SIGNAL(triggered(bool)), this,
          SLOT(showSettings()));
  connect(ui.action_About, SIGNAL(triggered(bool)), this, SLOT(showAbout()));

  // Call the init function after the main application loop starts.
  QTimer::singleShot(0, this, SLOT(init()));
}

void MainWindow::init() {
  // Add the clients to the menu.
  updateClientList(QSettings().value("clientList").toMap());

  // Create the socket for the live connection.
  mLiveSocket = new QTcpSocket;
  mLiveSocket->connectToHost(QHostAddress::LocalHost, 10676);

  // Create the logger server object.
  mServer = new LoggerServer;

  // Connect the signals from the logger server to the GUI.
  connect(mServer, SIGNAL(message(const QString &)), this,
          SLOT(addLogMessage(const QString &)));
  connect(mServer, SIGNAL(packet(const QByteArray &)), this,
          SLOT(addPacket(const QByteArray &)));

  // Set the capture path and start the logger server.
  mServer->setCapturePath(mCapturePath);
  mServer->startServer();
}

void MainWindow::addLogMessage(const QString &msg) {
  // Insert the message at the bottom of the log.
  QTextCursor cur = ui.logEdit->textCursor();
  cur.movePosition(QTextCursor::End);
  cur.insertText(msg);

  // Make sure the line ends with a new line character.
  if (msg.right(1) != "\n") cur.insertText("\n");

  // Scroll the cursor to the end of the log.
  cur.movePosition(QTextCursor::End);
}

void MainWindow::addPacket(const QByteArray &p) {
  // If there is a live connection, send the packet to capgrep.
  if (mLiveSocket->state() != QAbstractSocket::ConnectedState) return;

  mLiveSocket->write(p);
}

void MainWindow::showSettings() {
  // Create and show the settings dialog box.
  Settings *settings = new Settings(mServer, this);

  // If the client list has changed, update it.
  connect(settings, SIGNAL(clientListChanged(const QVariantMap &)), this,
          SLOT(updateClientList(const QVariantMap &)));

  settings->show();
}

void MainWindow::showCaptures() {
  // Show the captures directory in the native OS file browser.
  mCapturePath = QDir::toNativeSeparators(mCapturePath);

#ifdef Q_OS_WIN32
  QProcess::startDetached("explorer", QStringList() << mCapturePath);
#else   // Q_OS_WIN32
  QProcess::startDetached("/usr/bin/nautilus", QStringList() << mCapturePath);
#endif  // Q_OS_WIN32
}

void MainWindow::closeEvent(QCloseEvent *evt) {
  // Ignore the event.
  evt->ignore();

  // Shutdown the logger.
  shutdown();
}

void MainWindow::shutdown() {
  // Check if the close warning dialog box should be shown.
  if (QSettings().value("noexitwarning", false).toBool()) {
    // Quit the application.
    qApp->quit();
  } else {
    // Show the close warning dialog.
    (new Close)->show();
  }
}

QString MainWindow::serverLine() const {
  // By default, direct all clients back to localhost.
  return "127.0.0.1";
}

void MainWindow::startGame() {
  // This method must be called by an action signal for a client menu item.
  QAction *act = qobject_cast<QAction *>(sender());
  if (!act) return;

  // Retrieve the associated path for this client.
  QString path = act->data().toString();

  uint32_t ver;
  bool isUS;

  // Detect the client version.
  if (!versionCheck(tr("%1/ImagineClient.exe").arg(path), 0, &ver, &isUS)) {
    QMessageBox::critical(
        this, tr("Invalid Client"),
        tr("Failed to detect the client version, the client won't be started"));

    return;
  }

  // Update the client version setting with the version of the client.
  if (isUS)
    mServer->setVersionUS(ver);
  else
    mServer->setVersionJP(ver);

  // Ask the user if they would like to enable channel logging.
  QMessageBox msgBox(QMessageBox::Question, tr("Channel Logging"),
                     tr("Would you like to enable channel logging?"),
                     QMessageBox::NoButton, this);
  msgBox.setDefaultButton(
      msgBox.addButton(tr("Enable"), QMessageBox::AcceptRole));
  msgBox.addButton(tr("Disable"), QMessageBox::RejectRole);
  msgBox.exec();

  // Enable the channel logging setting if the "Enable" (default) button
  // was clicked.
  mServer->setChannelLogEnabled(msgBox.clickedButton() ==
                                msgBox.defaultButton());

  // Re-write the ImagineClient.dat to connect to the logger.
  QFile serverInfo(tr("%1/ImagineClient.dat").arg(path));
  serverInfo.open(QIODevice::WriteOnly);
  serverInfo.write(QString("-ip %1\r\n").arg(serverLine()).toUtf8());
  serverInfo.write(QString("-port %1\r\n").arg(10666).toUtf8());
  serverInfo.close();

  // Patch the webaccess.sdat file to connect to the logger if the WebAuth
  // option is enabled.
  if (!isUS && mServer->isWebAuthJPEnabled()) patchWebAccess(path);

#ifdef Q_OS_WIN32
  // If AppLocale is installed and the client is JP, start the client
  // executable with AppLocale; otherwise, start it normally.
  if (QFileInfo("C:\\Windows\\AppPatch\\AppLoc.exe").exists() && !isUS) {
    QProcess::startDetached(
        "C:\\Windows\\AppPatch\\AppLoc.exe",
        QStringList() << tr("%1\\ImagineClient.exe").arg(path) << "/L0411",
        path);
  } else {
    QProcess::startDetached(tr("%1\\ImagineClient.exe").arg(path),
                            QStringList(), path);
  }
#else   // Q_OS_UNIX
  // Start the client using WINE and change the language to Japanese if the
  // client is Japanese.
  if (isUS) {
    QProcess::startDetached("wine", QStringList() << "ImagineClient.exe", path);
  } else {
    QProcess::startDetached("bash",
                            QStringList()
                                << "-c"
                                << "export LANG=ja_JP.SJIS;nohup wine "
                                   "ImagineClient.exe &> /dev/null &",
                            path);
  }
#endif  // Q_OS_WIN32
}

void MainWindow::patchWebAccess(const QString &path) {
  QString webAccessPath = tr("%1/webaccess.sdat").arg(path);

  // Decrypt and load the file into a buffer.
  std::vector<char> webAccessData =
      libcomp::Crypto::DecryptFile(webAccessPath.toUtf8().constData());
  if (webAccessData.empty()) return;

  // Convert the file to a string for processing.
  QString xml = QString::fromUtf8(&webAccessData[0],
                                  static_cast<int>(webAccessData.size()));

  // Replace the login URL with the logger.
  xml.replace(QRegExp("\\<login\\s*\\=\\s*[^\\>]+\\>"),
              tr("<login = http://%1:10999/>").arg(serverLine()));

  // Convert the string back into binary.
  QByteArray out = xml.toUtf8();

  // Encrypt the data into a buffer.
  libcomp::Crypto::EncryptFile(
      webAccessPath.toUtf8().constData(),
      std::vector<char>(out.data(), out.data() + out.size()));
}

QList<quint32> MainWindow::findMatches(const void *haystack,
                                       quint32 haystack_size,
                                       const void *needle,
                                       quint32 needle_size) {
  QList<quint32> matches;

  haystack_size -= needle_size;

  const uchar *d = static_cast<const uchar *>(haystack);

  // Search for a needle in a haystack. Add all occurrences of the needle
  // to the list "matches".
  for (quint32 i = 0; i < haystack_size; i++) {
    if (memcmp(d + i, needle, needle_size) != 0) continue;

    matches << static_cast<quint32>(i);
  }

  return matches;
}

bool MainWindow::versionCheck(const QString &path, float *floatVer,
                              uint32_t *ver, bool *isUS) {
  // Open the client executable.
  QFile inFile(path);
  if (!inFile.open(QIODevice::ReadOnly)) return false;

  // Map the client executable into virtual memory.
  uint32_t inSize = (uint32_t)inFile.size();
  uchar *inMap = inFile.map(0, inSize, QFile::NoOptions);

  if (!inMap) return false;

  libcomp::PEFile pe(inMap);

  QList<quint32> matches;

  // Find the US version string.
  matches = findMatches(inMap, inSize, "IMAGINE Version %4.3fU", 22);

  // If the US version string was found, the client is US; otherwise, search
  // for the JP version string. If neither is found, the client is invalid.
  // If the JP version string was found, the client is JP.
  if (matches.count() != 1) {
    matches = findMatches(inMap, inSize, "IMAGINE Version %4.3f", 21);
    if (matches.count() != 1) return false;

    if (isUS) *isUS = false;
  } else if (isUS) {
    *isUS = true;
  }

  // Get the offset of the version string in the executable and convert it to
  // a virtual address.
  uint32_t verStrOffset = matches.first();
  uint32_t verStrAddress = pe.offsetToAddress(verStrOffset, ".rwdata");

  // Search for the address to find the code that references the string.
  matches = findMatches(inMap, inSize, &verStrAddress, 4);
  if (matches.count() != 1) return false;

  // Check for all the expected commands to make sure the section of code is
  // still valid.
  uint32_t pushOffset = matches.first() - 1;

  // Check for push.
  if (*(inMap + pushOffset) != 0x68) return false;

  bool isDouble = false;

  // Check for fld.
  if (*(inMap + pushOffset - 16) != 0xD9 ||
      *(inMap + pushOffset - 15) != 0x05) {
    // Check for a double instead.
    if (*(inMap + pushOffset - 16) != 0xDD) return false;
    if (*(inMap + pushOffset - 15) != 0x05) return false;

    isDouble = true;
  }

  // The version is either stored as a double or float. Either way, take the
  // address of the constant that needs to be loaded and convert it to a file
  // offset. Next, read that offset and store the resulting version number.
  uint32_t verAddress;
  memcpy(&verAddress, inMap + pushOffset - 14, 4);

  float version = 0.0f;
  if (isDouble) {
    uint32_t verOffset = pe.absoluteToOffset(verAddress, ".rdata");
    if (verOffset > inSize) return false;

    double doubleVer;
    memcpy(&doubleVer, inMap + verOffset, 8);

    version = (float)doubleVer;
  } else {
    uint32_t verOffset = pe.addressToOffset(verAddress, ".rwdata");
    if (verOffset > inSize) return false;

    memcpy(&version, inMap + verOffset, 4);
  }

  // Fill in the requested variables.
  if (ver) *ver = (uint32_t)(version * 1000.0f + 0.5f);
  if (floatVer) *floatVer = version;

  // Unmap and close the file.
  inFile.unmap(inMap);
  inFile.close();

  return true;
}

void MainWindow::showAbout() {
  // Open the license to add to the dialog box.
  QFile license(":/LICENSE");
  license.open(QIODevice::ReadOnly);

  // The UI to apply to the dialog.
  Ui::About aboutUI;

  // Create the dialog.
  QDialog *about = new QDialog(this);

  // Make sure the dialog is deleted.
  about->setAttribute(Qt::WA_DeleteOnClose);

  // Apply the UI and the license text.
  aboutUI.setupUi(about);
  aboutUI.licenseBox->setPlainText(license.readAll());

  // Show the about dialog.
  about->show();
}

void MainWindow::updateClientList(const QVariantMap &clientList) {
  QMapIterator<QString, QVariant> it(clientList);

  // Get all the actions in the start game menu and exclude the first 2 items
  // that should always remain in the menu.
  QList<QAction *> acts = ui.menuStart_Game->actions();
  acts.removeFirst();  // Add client...
  acts.removeFirst();  // Separator

  // Remove the actions from the menu and delete them.
  foreach (QAction *act, acts) {
    ui.menuStart_Game->removeAction(act);

    delete act;
  }

  // For each client listed in the settings, create a menu action with the
  // title of the client. Set the user data for the action to the path to the
  // client executable. Add the action to the menu and connect the signal to
  // the startGame method.
  while (it.hasNext()) {
    it.next();

    QString title = it.key();
    QString path = it.value().toString();

    QAction *act = ui.menuStart_Game->addAction(title);
    act->setData(path);

    connect(act, SIGNAL(triggered(bool)), this, SLOT(startGame()));
  }
}
