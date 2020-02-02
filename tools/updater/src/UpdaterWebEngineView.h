/**
 * @file tools/updater/src/UpdaterWebEngineView.h
 * @ingroup tools
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Web page viewer class.
 *
 * This tool will update the game client.
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

#ifndef TOOLS_UPDATER_SRC_UPDATERWEBENGINEVIEW_H
#define TOOLS_UPDATER_SRC_UPDATERWEBENGINEVIEW_H

#include <PushIgnore.h>
#include <QWebEngineView.h>
#include <PopIgnore.h>

class UpdaterWebEngineView : public QWebEngineView
{
    Q_OBJECT

public:
    UpdaterWebEngineView(QWidget *pParent = 0);
    ~UpdaterWebEngineView();

protected:
    virtual QWebEngineView* createWindow(QWebEnginePage::WebWindowType type);

protected slots:
    void openExternal(const QUrl& url);
};

#endif // TOOLS_UPDATER_SRC_UPDATERWEBENGINEVIEW_H
