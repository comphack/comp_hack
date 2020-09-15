/**
 * @file tools/logger/src/MainWindow.h
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

#ifndef TOOLS_LOGGER_SRC_MAINWINDOW_H
#define TOOLS_LOGGER_SRC_MAINWINDOW_H

#include <PopIgnore.h>
#include <PushIgnore.h>
#include <stdint.h>

#include "ui_MainWindow.h"

class LoggerServer;
class QTcpSocket;

/**
 * Main window for the COMP_hack Logger. This window provides the interface to
 * change settings on the logger as well as display feedback on the server
 * operation. Any informational or error messages will be displayed in this
 * window. Closing this window will terminate the application.
 */
class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  /**
   * Construct the main window class.
   * @arg capturePath Path to folder that capture files will be saved into.
   * @arg parent Parent widget that this window belongs to. Should remain 0.
   */
  MainWindow(const QString &capturePath, QWidget *parent = 0);

  /**
   * Determine the version of the client executable. For example, if the
   * client version is 1.684U then floatVer is 1.684, ver is 1684, and isUS
   * is true. If the client version is 1.441 then floatVer is 1.441, ver is
   * 1441 and isUS is false.
   * @arg path Path to the client executable to check.
   * @arg floatVer Pointer to a variable to store the version number in.
   * @arg ver Pointer to a variable to store the version number in.
   * @arg isUS Pointer to a variable to store whether the client is US or JP.
   * @returns true if the version number was determined, false if an error
   * occured.
   */
  static bool versionCheck(const QString &path, float *floatVer, uint32_t *ver,
                           bool *isUS);

  /**
   * Find all occurrences of a sequence of bytes in a larger byte buffer.
   * Why did I bother writing my own method for this? No clue ^_^.
   * @arg haystack Buffer to search in.
   * @arg haystack_size Size of the buffer to search in.
   * @arg needle Data to search for.
   * @arg needle_size Size of the data to search for.
   * @returns List of byte offsets in the buffer of matching sequences.
   */
  static QList<quint32> findMatches(const void *haystack, quint32 haystack_size,
                                    const void *needle, quint32 needle_size);

 public slots:
  /**
   * Display a new log message.
   * @arg msg Message to display.
   */
  void addLogMessage(const QString &msg);

  /**
   * If the logger is connected to a capgrep instance in live mode, add the
   * packet to the capgrep instance. This should only be used for channel
   * packets.
   * @arg p Packet to send to capgrep.
   */
  void addPacket(const QByteArray &p);

  /**
   * Display the settings window.
   */
  void showSettings();

  /**
   * Display the about dialog.
   */
  void showAbout();

  /**
   * Open the captures folder using the default file browser. On Windows,
   * this will open the folder in Windows Explorer. On Linux, this will open
   * the folder in Nautilus.
   */
  void showCaptures();

  /**
   * This slot must be called by a QAction in the start game sub-menu. The
   * requested client executable will be scanned for a version number. The
   * version setting for that client (US or JP) will be updated. The
   * ImagineClient.dat file will be modified to direct the client to the
   * logger. This file must be backed up or restored using the COMP_hack
   * updater. The client executable will then be started. If running Windows
   * and the client is JP, the application will attempt to use AppLocale to
   * change the locale to Japanese. If running Linux, the application will
   * start the client with WINE and set the locale to Japanese if required.
   */
  void startGame();

  /**
   * Give one last chance to back out before shutting down the logger.
   */
  void shutdown();

  /**
   * Initialize the server and start listening on the required ports. This
   * slot is called when the main event loop starts.
   */
  void init();

 protected slots:
  /**
   * Reload the client list in the start game sub-menu.
   * @arg clientList Mapping of client names to their paths. Both the keys
   * and values should be of the QString type.
   */
  void updateClientList(const QVariantMap &clientList);

 protected:
  /**
   * Patch the webaccess.sdat file to direct the client login page to the
   * logger. This method should only be called if you wish to use the WebAuth
   * feature of the logger. This feature is NOT needed or recommended.
   * @arg path Path to the webaccess.sdat file.
   */
  void patchWebAccess(const QString &path);

  /**
   * Get a string representing the IP address of the logger. This defaults to
   * 127.0.0.1 and may need to be changed in some cases.
   * @returns IP address of the logger.
   */
  QString serverLine() const;

  /**
   * When the main windows is closed, this method will be called. This method
   * will ensure the application exits.
   * @arg event Close event object.
   */
  virtual void closeEvent(QCloseEvent *event);

  /// Path to the directory capture files will be stored in.
  QString mCapturePath;

  /// Server object that controls all connections to the logger.
  LoggerServer *mServer;

  /// Socket for the live mode capgrep connection to send packets to.
  QTcpSocket *mLiveSocket;

  /// Generated class for the UI file.
  Ui::MainWindow ui;
};

#endif  // TOOLS_LOGGER_SRC_MAINWINDOW_H
