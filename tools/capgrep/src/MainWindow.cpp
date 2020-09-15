/**
 * @file tools/capgrep/src/MainWindow.cpp
 * @ingroup capgrep
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Main window definiton of the packet analysis application.
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

#include <PopIgnore.h>
#include <PushIgnore.h>

#include <QActionGroup>
#include <QClipboard>
#include <QDateTime>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QScrollBar>
#include <QSettings>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTextEdit>

#include "Filter.h"
#include "OpenMulti.h"
#include "Packets.h"
#include "Settings.h"
#include "ui_About.h"

// libcomp
#include <Convert.h>
#include <Endian.h>
#include <zlib.h>

static const uint32_t FORMAT_MAGIC = 0x4B434148;   // HACK
static const uint32_t FORMAT_MAGIC2 = 0x504D4F43;  // COMP
static const uint32_t FORMAT_VER1 = 0x00010000;  // Major, Minor, Patch (1.0.0)
static const uint32_t FORMAT_VER2 = 0x00010100;  // Major, Minor, Patch (1.1.0)

static int uncompressChunk(const void *src, void *dest, int in_size,
                           int chunk_size) {
  z_stream strm;

  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;

  strm.avail_in = static_cast<uInt>(in_size);
  strm.next_in = (Bytef *)src;

  if (inflateInit(&strm) != Z_OK) return 0;

  strm.avail_out = static_cast<uInt>(chunk_size);
  strm.next_out = (Bytef *)dest;

  if (inflate(&strm, Z_FINISH) != Z_STREAM_END) return 0;

  int written = chunk_size - static_cast<int>(strm.avail_out);

  if (inflateEnd(&strm) != Z_OK) return 0;

  return written;
}

static MainWindow *g_mainwindow = 0;

PacketListFilter *MainWindow::packetFilter() const { return mFilter; }

PacketListModel *MainWindow::packetModel() const { return mModel; }

PacketData *MainWindow::currentPacket() {
  return mModel->packetAt(
      mFilter->mapToSource(ui.packetList->currentIndex()).row());
}

MainWindow *MainWindow::getSingletonPtr() { return g_mainwindow; }

void MainWindow::addLogMessage(const QString &msg) {
  mDock->setVisible(true);

  QScrollBar *sbar = mLog->verticalScrollBar();

  bool scrollVisible = sbar->isVisible();
  bool atMaximum = sbar->sliderPosition() >= sbar->maximum();

  QTextCursor cur = mLog->textCursor();

  int curPos = cur.position();
  bool atEnd = cur.atEnd();

  cur.movePosition(QTextCursor::End);
  if (msg.right(1) == "\n")
    cur.insertText(msg);
  else
    cur.insertText(msg + "\n");

  if (atEnd)
    cur.movePosition(QTextCursor::End);
  else
    cur.setPosition(curPos);

  if (!scrollVisible || atMaximum) sbar->setSliderPosition(sbar->maximum());
}

MainWindow::MainWindow(QWidget *p)
    : QMainWindow(p),
      mFilter(new PacketListFilter),
      mModel(new PacketListModel),
      mLiveServer(0) {
  Q_ASSERT(g_mainwindow == 0);

  g_mainwindow = this;

  ui.setupUi(this);

  mDefaultState.servRate = 0;
  mDefaultState.lastTicks = 0;
  mDefaultState.nextTicks = 0;
  mDefaultState.lastUpdate = 0;
  mDefaultState.nextUpdate = 0;
  mDefaultState.packetSeqA = 0;
  mDefaultState.packetSeqB = 0;
  mDefaultState.client = -1;

  mFilter->setSourceModel(mModel);

  ui.packetData->setContextMenuPolicy(Qt::CustomContextMenu);
  ui.packetList->setContextMenuPolicy(Qt::CustomContextMenu);
  ui.packetList->setModel(mFilter);

  ui.actionStringEncoding->setSeparator(true);

  mStringEncodingGroup = new QActionGroup(this);
  mStringEncodingGroup->addAction(ui.actionStringCP1252);
  mStringEncodingGroup->addAction(ui.actionStringCP932);
  mStringEncodingGroup->addAction(ui.actionStringUTF8);

  mFindWindow = new Find(mFilter);

  updateRecentFiles();

  QSettings settings;

  // Determine if the scroll command list option is checked
  bool scrollSetting = settings.value("scroll", true).toBool();

  // Set the checkbox in the menu entry
  ui.actionScrollCommandList->setChecked(scrollSetting);

  // Toggle the feature on or off
  toggleScroll(scrollSetting);

  connect(mStringEncodingGroup, SIGNAL(triggered(QAction *)), this,
          SLOT(updateValues()));
  connect(ui.action_Quit, SIGNAL(triggered()), this, SLOT(close()));
  connect(ui.action_Open, SIGNAL(triggered()), this, SLOT(showOpenDialog()));
  connect(ui.action_OpenMulti, SIGNAL(triggered()), this,
          SLOT(showOpenMultiDialog()));
  connect(ui.actionPacketFilter, SIGNAL(triggered()), this,
          SLOT(showFiltersWindow()));
  connect(ui.action_Find, SIGNAL(triggered()), this, SLOT(showFindWindow()));
  connect(ui.action_Live_mode, SIGNAL(triggered()), this,
          SLOT(startLiveMode()));
  connect(ui.actionSettings, SIGNAL(triggered()), this, SLOT(showSettings()));
  connect(ui.actionScrollCommandList, SIGNAL(toggled(bool)), this,
          SLOT(toggleScroll(bool)));
  connect(
      ui.packetList->selectionModel(),
      SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
      this, SLOT(itemSelectionChanged()));
  connect(ui.actionAbout, SIGNAL(triggered()), this, SLOT(showAbout()));
  connect(ui.packetData, SIGNAL(selectionChanged()), this,
          SLOT(updateValues()));
  connect(ui.littleEndian, SIGNAL(toggled(bool)), this, SLOT(updateValues()));
  connect(ui.hexButton, SIGNAL(toggled(bool)), this, SLOT(updateValues()));
  connect(ui.packetData, SIGNAL(customContextMenuRequested(const QPoint &)),
          this, SLOT(packetContextMenu(const QPoint &)));
  connect(ui.packetList, SIGNAL(customContextMenuRequested(const QPoint &)),
          this, SLOT(listContextMenu(const QPoint &)));

  connect(ui.actionFile1, SIGNAL(triggered()), this, SLOT(actionRecentFile()));
  connect(ui.actionFile2, SIGNAL(triggered()), this, SLOT(actionRecentFile()));
  connect(ui.actionFile3, SIGNAL(triggered()), this, SLOT(actionRecentFile()));
  connect(ui.actionFile4, SIGNAL(triggered()), this, SLOT(actionRecentFile()));
  connect(ui.actionFile5, SIGNAL(triggered()), this, SLOT(actionRecentFile()));

  mContextMenu = new QMenu(this);
  mContextMenu->addAction(ui.actionFindSelected);
  mContextMenu->addSeparator();
  mContextMenu->addAction(ui.actionClipboardCP1252);
  mContextMenu->addAction(ui.actionClipboardCP932);
  mContextMenu->addAction(ui.actionClipboardUTF8);
  mContextMenu->addSeparator();
  mContextMenu->addAction(ui.actionClipboardCArray);
  mContextMenu->addAction(ui.actionClipboardHexDump);
  mContextMenu->addAction(ui.actionClipboardRawData);
  mContextMenu->addAction(ui.actionClipboardU32Array);

  mListContextMenu = new QMenu(this);
  mListContextMenu->addAction(ui.actionAddToBlackList);
  mListContextMenu->addAction(ui.actionAddToWhiteList);
  mListContextMenu->addAction(ui.actionCopyToClipboard);

  connect(ui.actionFindSelected, SIGNAL(triggered()), this,
          SLOT(actionFindSelected()));
  connect(ui.actionClipboardCP1252, SIGNAL(triggered()), this,
          SLOT(actionClipboardCP1252()));
  connect(ui.actionClipboardCP932, SIGNAL(triggered()), this,
          SLOT(actionClipboardCP932()));
  connect(ui.actionClipboardUTF8, SIGNAL(triggered()), this,
          SLOT(actionClipboardUTF8()));
  connect(ui.actionClipboardCArray, SIGNAL(triggered()), this,
          SLOT(actionClipboardCArray()));
  connect(ui.actionClipboardHexDump, SIGNAL(triggered()), this,
          SLOT(actionClipboardHexDump()));
  connect(ui.actionClipboardRawData, SIGNAL(triggered()), this,
          SLOT(actionClipboardRawData()));
  connect(ui.actionClipboardU32Array, SIGNAL(triggered()), this,
          SLOT(actionClipboardU32Array()));

  connect(ui.actionAddToBlackList, SIGNAL(triggered()), this,
          SLOT(actionAddToBlackList()));
  connect(ui.actionAddToWhiteList, SIGNAL(triggered()), this,
          SLOT(actionAddToWhiteList()));
  connect(ui.actionCopyToClipboard, SIGNAL(triggered()), this,
          SLOT(actionCopyToClipboard()));

  ui.packetDetails->setVisible(false);
  ui.statusLabel->setVisible(false);
  ui.line->setVisible(false);

  updateValues();

  mLog = new QTextEdit;
  mLog->setReadOnly(true);

  mDock = new QDockWidget(tr("Error Log"));
  mDock->setObjectName("errorLog");
  mDock->setVisible(false);
  mDock->setWidget(mLog);

  addDockWidget(Qt::BottomDockWidgetArea, mDock);

  ui.menu_View->insertAction(ui.actionPacketFilter, mDock->toggleViewAction());
  ui.menu_View->insertSeparator(ui.actionPacketFilter);

  // Restore the state and geometry of the window.
  restoreGeometry(settings.value("window_geom").toByteArray());
  restoreState(settings.value("window_state").toByteArray());
  ui.splitter->restoreGeometry(settings.value("splitter_geom").toByteArray());
  ui.splitter->restoreState(settings.value("splitter_state").toByteArray());

  QString encoding = settings.value("encoding", "cp1252").toString().toLower();

  if (encoding == "utf8")
    ui.actionStringUTF8->setChecked(true);
  else if (encoding == "cp932")
    ui.actionStringCP932->setChecked(true);
  else  // cp1252
    ui.actionStringCP1252->setChecked(true);

  if (settings.value("byte_order", "little").toString().toLower() == "big")
    ui.bigEndian->setChecked(true);

  if (settings.value("show_hex", false).toBool())
    ui.hexButton->setChecked(true);

  mStatusBar = new QLabel("statuslbl");
  mStatusBar->setText(tr("Ready - Open a capture file or enable live mode."));

  mModel->setPacketLimit(settings.value("packet_limit", 0).toInt());

  ui.statusbar->addPermanentWidget(mStatusBar, 1);
}

void MainWindow::updateRecentFiles() {
  QSettings settings;
  QStringList recentFiles;

  foreach (QString f, settings.value("recentFiles").toStringList()) {
    if (QFileInfo(f).exists()) recentFiles << f;
  }

  QList<QAction *> fileActions;
  fileActions << ui.actionFile1 << ui.actionFile2 << ui.actionFile3
              << ui.actionFile4 << ui.actionFile5;

  foreach (QAction *act, fileActions)
    act->setVisible(false);

  foreach (QString f, recentFiles) {
    QAction *act = fileActions.takeFirst();
    act->setText(QFileInfo(f).fileName());
    act->setVisible(true);
  }
}

void MainWindow::actionRecentFile() {
  QList<QAction *> fileActions;
  fileActions << ui.actionFile1 << ui.actionFile2 << ui.actionFile3
              << ui.actionFile4 << ui.actionFile5;

  int idx = fileActions.indexOf((QAction *)sender());
  if (idx < 0) return;

  QSettings settings;
  QStringList recentFiles = settings.value("recentFiles").toStringList();

  loadCapture(recentFiles.at(idx));
}

void MainWindow::addRecentFile(const QString &file) {
  QSettings settings;
  QStringList recentFiles = settings.value("recentFiles").toStringList();

  int idx = recentFiles.indexOf(file);
  if (idx >= 0) recentFiles.removeAt(idx);

  while (recentFiles.count() >= 5) recentFiles.removeLast();

  recentFiles.prepend(file);

  settings.setValue("recentFiles", recentFiles);

  updateRecentFiles();
}

void MainWindow::startLiveMode() {
  QSettings settings;

  // Limit the packets so live mode does not lag and turn on scrolling.
  settings.setValue("packet_limit", 100);
  settings.setValue("scroll", true);

  ui.action_Live_mode->setEnabled(false);

  delete mLiveServer;
  mLiveServer = 0;

  foreach (QTcpSocket *socket, mLiveSockets)
    socket->deleteLater();

  foreach (CaptureLoadState *state, mLiveStates)
    delete state;

  mLiveSockets.clear();
  mLiveStates.clear();

  mModel->clear();
  ui.packetData->setData(QByteArray());
  ui.packetDetails->clear();

  updateValues();

  mLiveServer = new QTcpServer;
  mLiveServer->listen(QHostAddress::LocalHost, 10676);

  connect(mLiveServer, SIGNAL(newConnection()), this, SLOT(newConnection()));

  setWindowTitle(tr("Capture Grep - Live Mode"));

  mStatusBar->setText("Live Mode");
}

void MainWindow::newConnection() {
  QTcpSocket *socket = mLiveServer->nextPendingConnection();
  if (!socket) return;

  mLiveSockets.append(socket);

  connect(socket, SIGNAL(readyRead()), this, SLOT(clientData()));
  connect(socket, SIGNAL(disconnected()), this, SLOT(clientLost()));

  // List how many connections there is
  setWindowTitle(tr("Capture Grep - Live Mode - %1 connection(s)")
                     .arg(mLiveSockets.count()));
}

void MainWindow::clientLost() {
  QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
  if (!socket) return;

  mLiveSockets.removeOne(socket);

  if (mLiveSockets.isEmpty()) {
    setWindowTitle(tr("Capture Grep - Live Mode"));
  } else {
    setWindowTitle(tr("Capture Grep - Live Mode - %1 connection(s)")
                       .arg(mLiveSockets.count()));
  }

  socket->close();
  socket->deleteLater();
}

void MainWindow::clientData() {
  QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
  if (!socket) return;

  while (socket->bytesAvailable() > 25) {
    libcomp::Packet p;
    p.WriteArray(socket->peek(25), 25);
    p.Rewind();
    p.Seek(21);

    uint32_t sz = p.ReadU32Little();

    // We don't have a full packet yet, wait for more data
    if (socket->bytesAvailable() < (sz + 25)) return;

    // Remove the header from the buffer
    p.Clear();
    p.WriteArray(socket->read(25).constData(), 25);
    p.Rewind();

    int32_t client = p.ReadS32Little();
    uint8_t source = p.ReadU8();
    uint64_t stamp = p.ReadU64Little();
    uint64_t micro = p.ReadU64Little();

    Q_ASSERT(p.ReadU32Little() == sz);

    CaptureLoadState *state = mLiveStates.value(client);
    if (!state) {
      state = new CaptureLoadState;
      state->servRate = 0;
      state->lastTicks = 0;
      state->nextTicks = 0;
      state->lastUpdate = 0;
      state->nextUpdate = 0;
      state->packetSeqA = 0;
      state->packetSeqB = 0;
      state->client = client % 6;

      mLiveStates[client] = state;
    }

    p.Clear();
    p.WriteArray(socket->read(sz).constData(), sz);
    p.Rewind();

    addPacket(source, stamp, micro, p, state);
  }
}

void MainWindow::loadCaptures(const QStringList &inPaths) {
  // Turn off the packet limiting so we can see the entire dump.
  QSettings().setValue("packet_limit", 0);

  ui.action_Live_mode->setEnabled(true);

  // Clear the log.
  mLog->clear();

  delete mLiveServer;
  mLiveServer = 0;

  foreach (QTcpSocket *socket, mLiveSockets)
    socket->deleteLater();

  foreach (CaptureLoadState *state, mLiveStates)
    delete state;

  mLiveSockets.clear();
  mLiveStates.clear();

  mModel->clear();
  ui.packetData->setData(QByteArray());
  ui.packetDetails->clear();

  updateValues();

  QList<CaptureLoadData *> capData;

  // Variable to store list of loaded PacketData objects
  QList<PacketData *> packetData;

  for (int i = 0; i < inPaths.count(); i++) {
    QString path = inPaths.at(i);
    if (path.isEmpty()) continue;

    addRecentFile(path);

    CaptureLoadState *state = new CaptureLoadState;
    state->servRate = 0;
    state->lastTicks = 0;
    state->nextTicks = 0;
    state->lastUpdate = 0;
    state->nextUpdate = 0;
    state->packetSeqA = 0;
    state->packetSeqB = 0;
    state->client = i;

    QFile *file = new QFile(path);
    if (!file->open(QIODevice::ReadOnly)) {
      foreach (CaptureLoadData *cap, capData) {
        delete cap->file;
        cap->file = nullptr;

        delete cap->state;
        cap->state = nullptr;

        delete cap;
        cap = nullptr;
      }

      QMessageBox::critical(this, tr("Capture File Error"),
                            tr("Failed to open the capture file."));

      return;
    }

    uint32_t magic = 0, ver = 0;

    file->read((char *)&magic, 4);
    file->read((char *)&ver, 4);

    if ((magic != FORMAT_MAGIC && magic != FORMAT_MAGIC2) ||
        (ver != FORMAT_VER1 && ver != FORMAT_VER2)) {
      foreach (CaptureLoadData *cap, capData) { delete cap; }

      QMessageBox::critical(this, tr("Capture File Error"),
                            tr("Invalid or corrupt capture file."));

      return;
    }

    uint64_t stamp = 0;
    uint32_t addrlen = 0;
    char address[1024];

    memset(address, 0, 1024);

    if (ver == FORMAT_VER1)
      file->read((char *)&stamp, 4);
    else
      file->read((char *)&stamp, 8);

    file->read((char *)&addrlen, 4);
    file->read(address, addrlen);

    CaptureLoadData *cap = new CaptureLoadData;
    cap->path = path;
    cap->file = file;
    cap->state = state;
    cap->ver = ver;

    if (!loadCapturePacket(cap))
      delete cap;
    else
      capData << cap;
  }

  while (!capData.isEmpty()) {
    int index = 0;
    uint64_t stamp = capData.first()->stamp;

    for (int i = 1; i < capData.count(); i++) {
      if (capData.at(i)->stamp >= stamp) continue;

      stamp = capData.at(i)->stamp;
      index = i;
    }

    CaptureLoadData *cap = capData.at(index);
    if (!cap) {
      capData.removeAt(index);

      continue;
    }

    libcomp::Packet p;

    p.Clear();
    p.WriteArray(cap->buffer, cap->sz);

    createPacketData(packetData, cap->source, cap->stamp, cap->micro, p,
                     cap->state);

    // Read in the next packet
    if (!loadCapturePacket(cap)) {
      capData.removeAt(index);

      delete cap;
    }
  }

  // This should be empty, but let's be safe
  foreach (CaptureLoadData *cap, capData)
    delete cap;

  // Add the final list of PacketData objects to the model in one shot
  mModel->addPacketData(packetData);

  setWindowTitle(tr("Capture Grep - Multiple Captures"));
}

bool MainWindow::loadCapturePacket(CaptureLoadData *d) {
  if (!d) return false;

  d->stamp = 0;
  d->micro = 0;

  if (d->file->atEnd()) return false;

  if (d->file->read((char *)&d->source, sizeof(d->source)) !=
      (qint64)sizeof(d->source))
    return false;

  if (d->ver == FORMAT_VER1) {
    if (d->file->read((char *)&d->stamp, 4) != 4) return false;
  } else {
    if (d->file->read((char *)&d->stamp, 8) != 8) return false;

    if (d->file->read((char *)&d->micro, 8) != 8) return false;
  }

  if (d->file->read((char *)&d->sz, sizeof(d->sz)) != (qint64)sizeof(d->sz))
    return false;

  if (d->file->read((char *)d->buffer, d->sz) != d->sz) return false;

  return true;
}

void MainWindow::loadCapture(const QString &path) {
  ui.action_Live_mode->setEnabled(true);

  mDefaultState.servRate = 0;
  mDefaultState.lastTicks = 0;
  mDefaultState.nextTicks = 0;
  mDefaultState.lastUpdate = 0;
  mDefaultState.nextUpdate = 0;
  mDefaultState.packetSeqA = 0;
  mDefaultState.packetSeqB = 0;
  mDefaultState.client = -1;

  // Clear the log.
  mLog->clear();

  addRecentFile(path);

  delete mLiveServer;
  mLiveServer = 0;

  foreach (QTcpSocket *socket, mLiveSockets)
    socket->deleteLater();

  foreach (CaptureLoadState *state, mLiveStates)
    delete state;

  mLiveSockets.clear();
  mLiveStates.clear();

  mModel->clear();
  ui.packetData->setData(QByteArray());
  ui.packetDetails->clear();

  updateValues();

  QFile log(path);

  // Open the log
  if (!log.open(QIODevice::ReadOnly)) {
    QMessageBox::critical(this, tr("Capture File Error"),
                          tr("Failed to open the capture file."));

    return;
  }

  uint32_t magic = 0, ver = 0;

  log.read((char *)&magic, 4);
  log.read((char *)&ver, 4);

  if ((magic != FORMAT_MAGIC && magic != FORMAT_MAGIC2) ||
      (ver != FORMAT_VER1 && ver != FORMAT_VER2)) {
    QMessageBox::critical(this, tr("Capture File Error"),
                          tr("Invalid or corrupt capture file."));

    return;
  }

  bool isLobby = (FORMAT_MAGIC2 == magic);

  uint64_t stamp = 0;
  uint64_t micro = 0;
  uint32_t addrlen = 0;
  char address[1024];

  memset(address, 0, 1024);

  if (ver == FORMAT_VER1)
    log.read((char *)&stamp, 4);
  else
    log.read((char *)&stamp, 8);

  log.read((char *)&addrlen, 4);
  log.read(address, addrlen);

  uint8_t *pBuffer = new uint8_t[1048576];
  uint8_t source;
  uint32_t sz;

  libcomp::Packet p;

  // Variable to store list of loaded PacketData objects
  QList<PacketData *> packetData;

  while (!log.atEnd()) {
    stamp = 0;
    micro = 0;

    log.read((char *)&source, sizeof(source));

    if (ver == FORMAT_VER1) {
      log.read((char *)&stamp, 4);
    } else {
      log.read((char *)&stamp, 8);
      log.read((char *)&micro, 8);
    }

    log.read((char *)&sz, sizeof(sz));
    log.read((char *)pBuffer, sz);

    p.Clear();
    p.WriteArray(pBuffer, sz);

    createPacketData(packetData, source, stamp, micro, p, isLobby);
  }

  log.close();

  // Add the final list of PacketData objects to the model in one shot
  mModel->addPacketData(packetData);

  setWindowTitle(tr("Capture Grep - %1").arg(QFileInfo(path).fileName()));

  mStatusBar->setText(QDir::toNativeSeparators(path));

  delete[] pBuffer;
}

void MainWindow::addPacket(uint8_t source, uint64_t stamp, uint64_t micro,
                           libcomp::Packet &p, CaptureLoadState *state) {
  // Variable to store the list of PacketData objects
  QList<PacketData *> packetData;

  // Create the PacketData objects
  createPacketData(packetData, source, stamp, micro, p, false, state);

  // Add the PacketData objects into the list model
  mModel->addPacketData(packetData);
}

void MainWindow::createPacketData(QList<PacketData *> &packetData,
                                  uint8_t source, uint64_t stamp,
                                  uint64_t micro, libcomp::Packet &p,
                                  bool isLobby, CaptureLoadState *state) {
  if (!state) state = &mDefaultState;

  if (mCopyActions.isEmpty()) {
    mCopyActions[0x0014] = &action0014;
    mCopyActions[0x0015] = &action0015;
    mCopyActions[0x0023] = &action0023;
    mCopyActions[0x00A7] = &action00A7;
    mCopyActions[0x00AC] = &action00AC;
    mCopyActions[0x00B9] = &action00B9;
  }

  p.Seek(8);

  // Check for compression
  if (!isLobby && p.ReadU32Big() == 0x677A6970) {
    int32_t uncompressed_size = p.ReadS32Little();
    int32_t compressed_size = p.ReadS32Little();

    uint32_t lv6 = p.ReadU32Big();

    (void)lv6;

    Q_ASSERT(lv6 == 0x6C763600);  // lv6

    if (compressed_size != uncompressed_size) {
      uint8_t *decomp = new uint8_t[uncompressed_size];

      int written = uncompressChunk(p.Data() + p.Tell(), decomp,
                                    compressed_size, uncompressed_size);

      libcomp::Packet dpacket;
      dpacket.WriteArray(p.Data(), p.Tell());
      dpacket.WriteArray(decomp, static_cast<uint32_t>(written));

      p.Clear();
      p.WriteArray(dpacket.Data(), dpacket.Size());

      delete[] decomp;
    }
  }

  p.Rewind();
  p.Skip(isLobby ? 8 : 24);

  while (p.Left() >= 6) {
    p.Skip(2);  // Big endian size

    uint32_t cmd_start = p.Tell();
    uint16_t cmd_size = p.ReadU16Little();
    if (cmd_size < 4) continue;

    PacketData *d = new PacketData;
    d->cmd = p.ReadU16Little();
    d->source = source;
    d->data = QByteArray(p.Data() + cmd_start + 4, cmd_size - 4);
    d->copyAction = 0;
    d->micro = micro;

    if (d->cmd == 0x00F3) {
      memcpy(&state->nextUpdate, d->data.constData(), 4);
    } else if (d->cmd == 0x00F4) {
      memcpy(&state->nextTicks, d->data.constData() + 4, 4);

      if ((state->nextUpdate - state->lastUpdate) != 0) {
        state->servRate =
            (float)(state->nextTicks - state->lastTicks) /
            (float)((state->nextUpdate - state->lastUpdate) * 1000);
      }

      state->lastTicks = state->nextTicks;
      state->lastUpdate = state->nextUpdate;

      state->nextTicks = 0;
      state->nextUpdate = 0;
    }

    d->servRate = state->servRate;
    d->servTime =
        (uint32_t)((float)state->lastTicks +
                   (((float)stamp - (float)state->lastUpdate) * d->servRate));

    if (mCopyActions.contains(d->cmd))
      d->copyAction = mCopyActions.value(d->cmd);

    if (d->shortName.isEmpty())
      d->text = tr("CMD%1").arg(d->cmd, 4, 16, QLatin1Char('0'));
    else
      d->text = d->shortName;

    if (d->desc.isEmpty()) {
      const PacketInfo *info = PacketListModel::getPacketInfo(d->cmd);

      if (info) d->desc = info->desc;
    }

    if (source == 0)
      d->seq = state->packetSeqA;
    else
      d->seq = state->packetSeqB;

    d->client = state->client;

    packetData.append(d);

    p.Seek(cmd_start + cmd_size);
  }

  if (source == 0)
    state->packetSeqA++;
  else
    state->packetSeqB++;
}

void MainWindow::itemSelectionChanged() {
  PacketData *d = currentPacket();

  if (!d) return;

  ui.packetData->setData(d->data);
  ui.packetDetails->setText(d->desc);
  ui.packetDetails->setVisible(!d->desc.isEmpty());
}

void MainWindow::showFindWindow() {
  mFindWindow->show();
  mFindWindow->activateWindow();
}

void MainWindow::showSelection(int packet, int start, int stop) {
  if (stop < start) stop = start;

  ui.packetList->setCurrentIndex(
      mFilter->mapFromSource(mModel->modelIndex(packet)));

  ui.packetData->setSelection(start, stop);
  ui.packetData->scrollToOffset(start);
  ui.packetData->scrollToOffset(stop);
}

void MainWindow::updateValues() {
  bool haveSelection = ui.packetData->startOffset() >= 0;

  ui.actionFindSelected->setEnabled(haveSelection);
  ui.actionClipboardCP1252->setEnabled(haveSelection);
  ui.actionClipboardCP932->setEnabled(haveSelection);
  ui.actionClipboardUTF8->setEnabled(haveSelection);
  ui.actionClipboardCArray->setEnabled(haveSelection);
  ui.actionClipboardHexDump->setEnabled(haveSelection);
  ui.actionClipboardRawData->setEnabled(haveSelection);
  ui.actionClipboardU32Array->setEnabled(haveSelection);

  if (ui.hexButton->checkState() == Qt::Checked) {
    updateHexValues();

    return;
  }

  ui.s8->clear();
  ui.u8->clear();
  ui.s16->clear();
  ui.u16->clear();
  ui.s32->clear();
  ui.u32->clear();
  ui.f32->clear();
  ui.s64->clear();
  ui.u64->clear();
  ui.f64->clear();
  ui.time->clear();
  ui.binary->clear();
  ui.statusLabel->clear();

  bool big = ui.bigEndian->isChecked();

  PacketData *d = currentPacket();

  if (!d) return;

  int start = ui.packetData->startOffset();
  int stop = ui.packetData->stopOffset();

  int sz = stop - start + 1;
  int left = d->data.size() - start;

  QString selectionStr;
  {
    char *buffer = new char[sz + 1];
    memset(buffer, 0, static_cast<size_t>(sz + 1));
    memcpy(buffer, d->data.constData() + start, static_cast<size_t>(sz));

    QAction *act = mStringEncodingGroup->checkedAction();

    if (act == ui.actionStringCP1252) {
      selectionStr = QString::fromUtf8(
          libcomp::Convert::FromEncoding(libcomp::Convert::ENCODING_CP1252,
                                         std::vector<char>(buffer, buffer + sz))
              .C());
    } else if (act == ui.actionStringCP932) {
      selectionStr = QString::fromUtf8(
          libcomp::Convert::FromEncoding(libcomp::Convert::ENCODING_CP932,
                                         std::vector<char>(buffer, buffer + sz))
              .C());
    } else {
      selectionStr = QString::fromUtf8(buffer);
    }

    if (!selectionStr.isEmpty())
      selectionStr = tr(" - String: %1").arg(selectionStr);

    delete[] buffer;
  }

  ui.statusLabel->setVisible(start >= 0);
  ui.line->setVisible(start >= 0);

  if (sz > 1) {
    ui.statusLabel->setText(QString("Offset: %1 - %2 (%3 bytes)%4")
                                .arg(start)
                                .arg(stop)
                                .arg(sz)
                                .arg(selectionStr));
  } else {
    ui.statusLabel->setText(QString("Offset: %1").arg(start));
  }

  if (start < 0 || left < 1) return;

  int8_t s8;
  uint8_t u8;

  memcpy(&s8, d->data.constData() + start, sizeof(int8_t));
  memcpy(&u8, d->data.constData() + start, sizeof(uint8_t));

  ui.s8->setText(QString::number((int)s8, 10));
  ui.u8->setText(QString::number((uint)u8, 10));
  ui.binary->setText(QString("%1").arg((uint)u8, 8, 2, QLatin1Char('0')));

  if (left < 2) return;

  int16_t s16;
  uint16_t u16;

  memcpy(&s16, d->data.constData() + start, sizeof(int16_t));
  memcpy(&u16, d->data.constData() + start, sizeof(uint16_t));

  if (big) {
    u16 = be16toh(u16);
    memcpy(&s16, &u16, sizeof(uint16_t));
  }

  ui.s16->setText(QString::number((int)s16, 10));
  ui.u16->setText(QString::number((uint)u16, 10));

  if (left < 4) return;

  int32_t s32;
  uint32_t u32;
  float f32;

  memcpy(&s32, d->data.constData() + start, sizeof(int32_t));
  memcpy(&u32, d->data.constData() + start, sizeof(uint32_t));
  memcpy(&f32, d->data.constData() + start, sizeof(float));

  ui.f32->setText(QString::number(f32));
  ui.time->setText(QDateTime::fromTime_t(u32).toString(Qt::ISODate));

  if (big) {
    u32 = be32toh(u32);
    memcpy(&s32, &u32, sizeof(uint32_t));
  }

  ui.s32->setText(QString::number((int)s32, 10));
  ui.u32->setText(QString::number((uint)u32, 10));

  if (left < 8) return;

  int64_t s64;
  uint64_t u64;
  double f64;

  memcpy(&s64, d->data.constData() + start, sizeof(int64_t));
  memcpy(&u64, d->data.constData() + start, sizeof(uint64_t));
  memcpy(&f64, d->data.constData() + start, sizeof(double));

  ui.f64->setText(QString::number(f64));

  if (big) {
    u64 = be64toh(u64);
    memcpy(&s64, &u64, sizeof(uint64_t));
  }

  ui.s64->setText(QString::number((qlonglong)s64, 10));
  ui.u64->setText(QString::number((qulonglong)u64, 10));
}

void MainWindow::updateHexValues() {
  ui.s8->clear();
  ui.u8->clear();
  ui.s16->clear();
  ui.u16->clear();
  ui.s32->clear();
  ui.u32->clear();
  ui.f32->clear();
  ui.s64->clear();
  ui.u64->clear();
  ui.f64->clear();
  ui.time->clear();
  ui.binary->clear();
  ui.statusLabel->clear();

  bool big = ui.bigEndian->isChecked();

  PacketData *d = currentPacket();

  if (!d) return;

  int start = ui.packetData->startOffset();
  int stop = ui.packetData->stopOffset();

  int sz = stop - start + 1;
  int left = d->data.size() - start;

  QString selectionStr;
  {
    char *buffer = new char[sz + 1];
    memset(buffer, 0, static_cast<size_t>(sz + 1));
    memcpy(buffer, d->data.constData() + start, static_cast<size_t>(sz));

    QAction *act = mStringEncodingGroup->checkedAction();

    if (act == ui.actionStringCP1252) {
      selectionStr = QString::fromUtf8(
          libcomp::Convert::FromEncoding(libcomp::Convert::ENCODING_CP1252,
                                         std::vector<char>(buffer, buffer + sz))
              .C());
    } else if (act == ui.actionStringCP932) {
      selectionStr = QString::fromUtf8(
          libcomp::Convert::FromEncoding(libcomp::Convert::ENCODING_CP932,
                                         std::vector<char>(buffer, buffer + sz))
              .C());
    } else {
      selectionStr = QString::fromUtf8(buffer);
    }

    if (!selectionStr.isEmpty())
      selectionStr = tr(" - String: %1").arg(selectionStr);

    delete[] buffer;
  }

  ui.statusLabel->setVisible(start >= 0);
  ui.line->setVisible(start >= 0);

  if (sz > 1) {
    ui.statusLabel->setText(QString("Offset: 0x%1 - "
                                    "0x%2 (%3 bytes)%4")
                                .arg(start, 8, 16, QLatin1Char('0'))
                                .arg(stop, 8, 16, QLatin1Char('0'))
                                .arg(sz)
                                .arg(selectionStr));
  } else {
    ui.statusLabel->setText(
        QString("Offset: 0x%1").arg(start, 8, 16, QLatin1Char('0')));
  }

  if (start < 0 || left < 1) return;

  int8_t s8;
  uint8_t u8;

  memcpy(&s8, d->data.constData() + start, sizeof(int8_t));
  memcpy(&u8, d->data.constData() + start, sizeof(uint8_t));

  ui.s8->setText("N/A");
  ui.u8->setText(QString("0x%1").arg((uint)u8, 2, 16, QLatin1Char('0')));
  ui.binary->setText(QString("%1").arg((uint)u8, 8, 2, QLatin1Char('0')));

  if (left < 2) return;

  int16_t s16;
  uint16_t u16;

  memcpy(&s16, d->data.constData() + start, sizeof(int16_t));
  memcpy(&u16, d->data.constData() + start, sizeof(uint16_t));

  if (big) {
    u16 = be16toh(u16);
    memcpy(&s16, &u16, sizeof(uint16_t));
  }

  ui.s16->setText("N/A");
  ui.u16->setText(QString("0x%1").arg((uint)u16, 4, 16, QLatin1Char('0')));

  if (left < 4) return;

  int32_t s32;
  uint32_t u32;
  float f32;

  memcpy(&s32, d->data.constData() + start, sizeof(int32_t));
  memcpy(&u32, d->data.constData() + start, sizeof(uint32_t));
  memcpy(&f32, d->data.constData() + start, sizeof(float));

  ui.f32->setText(QString::number(f32));
  ui.time->setText(QDateTime::fromTime_t(u32).toString(Qt::ISODate));

  if (big) {
    u32 = be32toh(u32);
    memcpy(&s32, &u32, sizeof(uint32_t));
  }

  ui.s32->setText("N/A");
  ui.u32->setText(QString("0x%1").arg((uint)u32, 8, 16, QLatin1Char('0')));

  if (left < 8) return;

  int64_t s64;
  uint64_t u64;
  double f64;

  memcpy(&s64, d->data.constData() + start, sizeof(int64_t));
  memcpy(&u64, d->data.constData() + start, sizeof(uint64_t));
  memcpy(&f64, d->data.constData() + start, sizeof(double));

  ui.f64->setText(QString::number(f64));

  if (big) {
    u64 = be64toh(u64);
    memcpy(&s64, &u64, sizeof(uint64_t));
  }

  ui.s64->setText("N/A");
  ui.u64->setText(
      QString("0x%1").arg((qulonglong)u64, 16, 16, QLatin1Char('0')));
}

void MainWindow::showOpenDialog() {
  QString path =
      QFileDialog::getOpenFileName(this, tr("Open Capture File"), QString(),
                                   tr("COMP_hack Channel Capture (*.hack)\n"
                                      "COMP_hack Lobby Capture (*.comp)"));

  if (path.isEmpty()) return;

  loadCapture(path);
}

void MainWindow::showOpenMultiDialog() {
  OpenMulti *dialog = new OpenMulti(this);
  dialog->show();

  connect(dialog, SIGNAL(filesReady(const QStringList &)), this,
          SLOT(loadCaptures(const QStringList &)));
}

void MainWindow::packetContextMenu(const QPoint &pt) {
  mContextMenu->popup(ui.packetData->mapToGlobal(pt));
}

void MainWindow::showFiltersWindow() { (new Filter(this))->show(); }

void MainWindow::actionFindSelected() {
  PacketData *d = currentPacket();

  if (!d) return;

  int start = ui.packetData->startOffset();
  int stop = ui.packetData->stopOffset();
  int sz = stop - start + 1;

  mFindWindow->findTerm(d->data.mid(start, sz));

  showFindWindow();
}

void MainWindow::actionClipboardCP1252() {
  PacketData *d = currentPacket();

  if (!d) return;

  int start = ui.packetData->startOffset();
  int stop = ui.packetData->stopOffset();
  int sz = stop - start + 1;

  char *buffer = new char[sz + 1];
  memset(buffer, 0, static_cast<size_t>(sz + 1));
  memcpy(buffer, d->data.constData() + start, static_cast<size_t>(sz));

  qApp->clipboard()->setText(QString::fromUtf8(
      libcomp::Convert::FromEncoding(libcomp::Convert::ENCODING_CP1252,
                                     std::vector<char>(buffer, buffer + sz))
          .C()));

  delete[] buffer;
}

void MainWindow::actionClipboardCP932() {
  PacketData *d = currentPacket();

  if (!d) return;

  int start = ui.packetData->startOffset();
  int stop = ui.packetData->stopOffset();
  int sz = stop - start + 1;

  char *buffer = new char[sz + 1];
  memset(buffer, 0, static_cast<size_t>(sz + 1));
  memcpy(buffer, d->data.constData() + start, static_cast<size_t>(sz));

  qApp->clipboard()->setText(QString::fromUtf8(
      libcomp::Convert::FromEncoding(libcomp::Convert::ENCODING_CP932,
                                     std::vector<char>(buffer, buffer + sz))
          .C()));

  delete[] buffer;
}

void MainWindow::actionClipboardUTF8() {
  PacketData *d = currentPacket();

  if (!d) return;

  int start = ui.packetData->startOffset();
  int stop = ui.packetData->stopOffset();
  int sz = stop - start + 1;

  char *buffer = new char[sz + 1];
  memset(buffer, 0, static_cast<size_t>(sz + 1));
  memcpy(buffer, d->data.constData() + start, static_cast<size_t>(sz));

  qApp->clipboard()->setText(QString::fromUtf8(buffer));

  delete[] buffer;
}

void MainWindow::actionClipboardCArray() {
  PacketData *d = currentPacket();

  if (!d) return;

  int32_t start = ui.packetData->startOffset();
  int32_t stop = ui.packetData->stopOffset();
  int32_t sz = stop - start + 1;
  char buffer[80];

  const uint8_t *cdata = (const uint8_t *)d->data.constData() + start;

  QString final = tr("uint8_t untitled[%1] = {\n").arg(sz);

  for (int32_t line = 0; line < sz; line += 8) {
    char *bufferp = buffer;

    bufferp += sprintf(bufferp, "\t");

    for (int32_t i = line; i < (line + 7) && i < (sz - 1); i++)
      bufferp += sprintf(bufferp, "0x%02X, ", cdata[i]);

    if ((line + 8) >= sz)
      bufferp += sprintf(bufferp, "0x%02X\n", cdata[sz - 1]);
    else
      bufferp += sprintf(bufferp, "0x%02X,\n", cdata[line + 7]);

    final += QString::fromUtf8(buffer, (int)(bufferp - buffer));
  }

  final += "};";

  qApp->clipboard()->setText(final);
}

void MainWindow::actionClipboardHexDump() {
  PacketData *d = currentPacket();

  if (!d) return;

  QString final;

  int32_t start = ui.packetData->startOffset();
  int32_t stop = ui.packetData->stopOffset();
  int32_t sz = stop - start + 1;
  int32_t line = 0;
  char buffer[75];

  const uint8_t *cdata = (const uint8_t *)d->data.constData() + start;

  while (line < sz) {
    char *bufferp = buffer;

    // Print line number
    bufferp += sprintf(bufferp, "%04X  ", line);

    for (int32_t i = line; i < (line + 8); i++) {
      if (i >= sz)
        bufferp += sprintf(bufferp, "   ");
      else
        bufferp += sprintf(bufferp, "%02X ", cdata[i]);
    }
    bufferp += sprintf(bufferp, " ");

    for (int32_t i = (line + 8); i < (line + 16); i++) {
      if (i >= sz)
        bufferp += sprintf(bufferp, "   ");
      else
        bufferp += sprintf(bufferp, "%02X ", cdata[i]);
    }
    bufferp += sprintf(bufferp, " ");

    for (int32_t i = line; i < (line + 8) && i < sz; i++) {
      uint8_t val = cdata[i];
      bufferp +=
          sprintf(bufferp, "%c", (val >= 0x20 && val < 0x7f) ? val : '.');
    }
    bufferp += sprintf(bufferp, " ");

    for (int32_t i = (line + 8); i < (line + 16) && i < sz; i++) {
      uint8_t val = cdata[i];
      bufferp +=
          sprintf(bufferp, "%c", (val >= 0x20 && val < 0x7f) ? val : '.');
    }

    line += 16;

    bufferp += sprintf(bufferp, "\n");

    final += QString::fromUtf8(buffer, (int)(bufferp - buffer));
  }

  qApp->clipboard()->setText(final);
}

void MainWindow::actionClipboardRawData() {
  PacketData *d = currentPacket();

  if (!d) return;

  int start = ui.packetData->startOffset();
  int stop = ui.packetData->stopOffset();
  int sz = stop - start + 1;

  QMimeData *bytes = new QMimeData;
  bytes->setData("application/octet-stream", d->data.mid(start, sz));

  qApp->clipboard()->setMimeData(bytes);
}

void MainWindow::listContextMenu(const QPoint &pt) {
  mListContextItem = mFilter->mapToSource(ui.packetList->indexAt(pt));
  if (!mListContextItem.isValid()) return;

  PacketData *d = mModel->packetAt(mListContextItem.row());
  if (!d) return;

  ui.actionCopyToClipboard->setVisible(nullptr != d->copyAction);

  mListContextMenu->popup(ui.packetList->mapToGlobal(pt));
}

void MainWindow::actionAddToBlackList() {
  if (!mListContextItem.isValid()) return;

  PacketData *d = mModel->packetAt(mListContextItem.row());
  if (!d) return;

  mFilter->addBlack(d->cmd);
}

void MainWindow::actionAddToWhiteList() {
  if (!mListContextItem.isValid()) return;

  PacketData *d = mModel->packetAt(mListContextItem.row());
  if (!d) return;

  mFilter->addWhite(d->cmd);
}

void MainWindow::actionCopyToClipboard() {
  if (!mListContextItem.isValid()) return;

  PacketData *d = mModel->packetAt(mListContextItem.row());
  if (!d) return;

  if (!d->copyAction) return;

  libcomp::Packet packet;
  packet.WriteArray(d->data.constData(), static_cast<uint32_t>(d->data.size()));
  packet.Rewind();

  libcomp::Packet packetBefore;

  PacketData *beforeData = mModel->packetBefore(mListContextItem.row());
  if (beforeData) {
    packetBefore.WriteArray(beforeData->data.constData(),
                            static_cast<uint32_t>(beforeData->data.size()));
    packetBefore.Rewind();
  }

  (*d->copyAction)(d, packet, packetBefore);
}

void MainWindow::actionClipboardU32Array() {
  PacketData *d = currentPacket();

  if (!d) return;

  int32_t start = ui.packetData->startOffset();
  int32_t stop = ui.packetData->stopOffset();
  int32_t sz = stop - start + 1;

  if (sz % 4 != 0) return;

  sz /= 4;

  const uint8_t *cdata = (const uint8_t *)d->data.constData() + start;
  const uint32_t *values = (const uint32_t *)cdata;

  QString final = "uint32_t untitled[] = {\n";

  for (int32_t i = 0; i < (sz - 1); i++)
    final += QString("\t%1,\n").arg(values[i]);

  final += QString("\t%1\n};").arg(values[sz - 1]);

  qApp->clipboard()->setText(final);
}

void MainWindow::showSettings() {
  Settings *w = new Settings(this);

  connect(w, SIGNAL(packetLimitChanged(int)), this,
          SLOT(packetLimitChanged(int)));

  w->show();
}

void MainWindow::toggleScroll(bool checked) {
  QSettings settings;
  settings.setValue("scroll", checked);

  if (checked) {
    connect(mModel, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
            ui.packetList, SLOT(scrollToBottom()));
  } else {
    disconnect(mModel, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
               ui.packetList, SLOT(scrollToBottom()));
  }
}

void MainWindow::closeEvent(QCloseEvent *evt) {
  mFindWindow->close();

  QSettings settings;
  settings.setValue("window_geom", saveGeometry());
  settings.setValue("window_state", saveState());
  settings.setValue("splitter_geom", ui.splitter->saveGeometry());
  settings.setValue("splitter_state", ui.splitter->saveState());

  if (mStringEncodingGroup->checkedAction() == ui.actionStringCP1252)
    settings.setValue("encoding", "cp1252");
  else if (mStringEncodingGroup->checkedAction() == ui.actionStringCP932)
    settings.setValue("encoding", "cp932");
  else
    settings.setValue("encoding", "utf8");

  settings.setValue("byte_order",
                    ui.littleEndian->isChecked() ? "little" : "big");
  settings.setValue("show_hex", ui.hexButton->isChecked());

  QMainWindow::closeEvent(evt);
}

void MainWindow::packetLimitChanged(int limit) {
  mModel->setPacketLimit(limit);
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
