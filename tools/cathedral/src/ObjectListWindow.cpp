/**
 * @file tools/cathedral/src/ObjectListWindow.h
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Implementation for a window that holds a list of objgen objects.
 *
 * Copyright (C) 2012-2018 COMP_hack Team <compomega@tutanota.com>
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

#include "ObjectListWindow.h"

// Cathedral Includes
#include "ObjectListModel.h"

// Qt Includes
#include <PushIgnore.h>
#include "ui_ObjectListWindow.h"

#include <QSortFilterProxyModel>
#include <PopIgnore.h>

// libcomp Includes
#include <Log.h>

ObjectListWindow::ObjectListWindow(QWidget *pParent) :
    QWidget(pParent)
{
    mObjectModel = new ObjectListModel(this);

    mFilterModel = new QSortFilterProxyModel;
    mFilterModel->setSourceModel(mObjectModel);
    mFilterModel->setFilterRegExp(QRegExp("", Qt::CaseInsensitive,
        QRegExp::FixedString));
    mFilterModel->setFilterKeyColumn(0);

    ui = new Ui::ObjectListWindow;
    ui->setupUi(this);

    ui->objectList->setModel(mFilterModel);

    connect(ui->objectSearch, SIGNAL(textChanged(const QString&)),
        this, SLOT(Search(const QString&)));
    connect(ui->objectList->selectionModel(), SIGNAL(selectionChanged(
        const QItemSelection&, const QItemSelection&)),
        this, SLOT(SelectedObjectChanged()));
}

ObjectListWindow::~ObjectListWindow()
{
    delete ui;
}

void ObjectListWindow::SetMainWindow(MainWindow *pMainWindow)
{
    mMainWindow = pMainWindow;
}

void ObjectListWindow::Search(const QString& term)
{
    mFilterModel->setFilterRegExp(QRegExp(term, Qt::CaseInsensitive,
        QRegExp::FixedString));
}

QString ObjectListWindow::GetObjectName(const std::shared_ptr<
    libcomp::Object>& obj) const
{
    (void)obj;

    return  {};
}

void ObjectListWindow::SetObjectList(const std::vector<
    std::shared_ptr<libcomp::Object>>& objs)
{
    mObjectModel->SetObjectList(objs);
}

void ObjectListWindow::LoadProperties(
    const std::shared_ptr<libcomp::Object>& obj)
{
    (void)obj;
}

void ObjectListWindow::SaveProperties(
    const std::shared_ptr<libcomp::Object>& obj)
{
    (void)obj;
}

void ObjectListWindow::SelectedObjectChanged()
{
    auto obj = mActiveObject.lock();

    if(obj)
    {
        SaveProperties(obj);
    }

    auto idxList = ui->objectList->selectionModel()->selectedIndexes();

    if(idxList.isEmpty())
    {
        mActiveObject = {};
    }
    else
    {
        mActiveObject = mObjectModel->GetObject(
            mFilterModel->mapToSource(idxList.at(0)));
    }

    LoadProperties(mActiveObject.lock());
}

std::map<uint32_t, QString> ObjectListWindow::GetObjectMapping() const
{
    std::map<uint32_t, QString> mapping;

    int count = mObjectModel->rowCount();

    for(int i = 0; i < count; ++i)
    {
        auto idx = mObjectModel->index(i);
        auto obj = mObjectModel->GetObject(idx);

        mapping[GetObjectID(obj).toUInt()] = mObjectModel->data(idx).toString();
    }

    return mapping;
}
