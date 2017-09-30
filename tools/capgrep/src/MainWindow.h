/**
 * @file tools/capgrep/src/MainWindow.h
 * @ingroup capgrep
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Main window definiton of the packet analysis application.
 *
 * Copyright (C) 2010-2016 COMP_hack Team <compomega@tutanota.com>
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

#ifndef TOOLS_CAPGREP_SRC_MAINWINDOW_H
#define TOOLS_CAPGREP_SRC_MAINWINDOW_H

#include <PushIgnore.h>
#include "ui_MainWindow.h"
#include <PopIgnore.h>

#include "PacketListFilter.h"
#include "PacketListModel.h"
#include "PacketData.h"
#include "Packet.h"
#include "Find.h"

#include <PushIgnore.h>
#include <QFile>
#include <QHash>
#include <QList>
#include <PopIgnore.h>

#include <stdint.h>

class Find;

class QMenu;
class QTextEdit;
class QTcpServer;
class QTcpSocket;
class QDockWidget;
class QListWidgetItem;

class CaptureLoadState
{
public:
    float servRate;
    uint16_t packetSeqA, packetSeqB;
    uint32_t lastTicks, nextTicks;
    time_t lastUpdate, nextUpdate;
    int client;
};

class CaptureLoadData
{
public:
    ~CaptureLoadData() {
        delete file;
        delete state;
    }

    QFile *file;
    QString path;
    uint32_t ver;
    uint64_t stamp;
    uint64_t micro;
    CaptureLoadState *state;
    uint8_t buffer[1048576];
    uint8_t source;
    uint32_t sz;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);

    static MainWindow* getSingletonPtr();

    void loadCapture(const QString& path);
    void showSelection(int packet, int start = -1, int stop = -1);

    PacketData* currentPacket();
    PacketListFilter* packetFilter() const;
    PacketListModel* packetModel() const;

    void addLogMessage(const QString& msg);

protected slots:
    void loadCaptures(const QStringList& paths);
    bool loadCapturePacket(CaptureLoadData *d);
    void packetContextMenu(const QPoint& pt);
    void listContextMenu(const QPoint& pt);

    void addPacket(uint8_t source, uint64_t stamp, uint64_t micro,
        libcomp::Packet& p, CaptureLoadState *state = 0);
    void createPacketData(QList<PacketData*>& packetData, uint8_t source,
        uint64_t stamp, uint64_t micro, libcomp::Packet& p,
        bool isLobby, CaptureLoadState *state = 0);

    void packetLimitChanged(int limit);

    void showAbout();
    void showSettings();

    void updateRecentFiles();
    void actionRecentFile();

    void addRecentFile(const QString& file);

    void itemSelectionChanged();
    void showFiltersWindow();
    void showFindWindow();

    void showOpenDialog();
    void showOpenMultiDialog();

    void updateValues();
    void updateHexValues();

    void startLiveMode();
    void newConnection();
    void clientLost();
    void clientData();

    void actionFindSelected();
    void actionClipboardCP1252();
    void actionClipboardCP932();
    void actionClipboardUTF8();
    void actionClipboardCArray();
    void actionClipboardHexDump();
    void actionClipboardRawData();
    void actionClipboardU32Array();

    void actionAddToBlackList();
    void actionAddToWhiteList();
    void actionCopyToClipboard();

    void toggleScroll(bool checked);

protected:
    virtual void closeEvent(QCloseEvent *evt);

    Ui::MainWindow ui;

    PacketListFilter *mFilter;
    PacketListModel *mModel;

    Find *mFindWindow;
    QMenu *mContextMenu;
    QMenu *mListContextMenu;
    QLabel *mStatusBar;

    QTextEdit *mLog;
    QDockWidget *mDock;

    QTcpServer *mLiveServer;
    QList<QTcpSocket*> mLiveSockets;

    QModelIndex mListContextItem;
    QActionGroup *mStringEncodingGroup;
    QHash<uint32_t, CopyFunc> mCopyActions;
    QMap<int32_t, CaptureLoadState*> mLiveStates;

    CaptureLoadState mDefaultState;
};

#endif // TOOLS_CAPGREP_SRC_MAINWINDOW_H
