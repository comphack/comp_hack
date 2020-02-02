/**
 * @file tools/updater/src/UpdaterWebEngineView.cpp
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

#include "UpdaterWebEngineView.h"

#include <PushIgnore.h>
#include <QDesktopServices.h>
#include <PopIgnore.h>

UpdaterWebEngineView::UpdaterWebEngineView(QWidget *pParent) :
    QWebEngineView(pParent)
{
}

UpdaterWebEngineView::~UpdaterWebEngineView()
{
}

QWebEngineView* UpdaterWebEngineView::createWindow(
    QWebEnginePage::WebWindowType type)
{
    Q_UNUSED(type);

    UpdaterWebEngineView *pView = new UpdaterWebEngineView;
    pView->setAttribute(Qt::WA_DeleteOnClose);
    pView->show();

    connect(pView, SIGNAL(urlChanged(const QUrl&)),
        pView, SLOT(openExternal(const QUrl&)));
    
    return pView;
}

void UpdaterWebEngineView::openExternal(const QUrl& url)
{
    QDesktopServices::openUrl(url);
    deleteLater();
}
