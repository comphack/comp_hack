/**
 * @file tools/cathedral/src/ObjectSelectorWindow.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for a value selection window bound to an
 *  ObjectSelector.
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

#include "ObjectSelectorWindow.h"

// Cathedral Includes
#include "FindRefWindow.h"
#include "MainWindow.h"
#include "ObjectSelectorBase.h"
#include "ObjectSelectorList.h"

// Qt Includes
#include <PushIgnore.h>
#include <QCloseEvent>
#include "ui_ObjectSelectorWindow.h"
#include <PopIgnore.h>

ObjectSelectorWindow::ObjectSelectorWindow(MainWindow* pMainWindow,
    QWidget *pParent) : QMainWindow(pParent), mMainWindow(pMainWindow),
    mSelectorControl(0), mFindWindow(0)
{
    ui = new Ui::ObjectSelectorWindow;
    ui->setupUi(this);

    ui->select->setDisabled(true);

    // Hide find button by default
    ui->find->hide();

    connect(ui->select, SIGNAL(clicked()), this, SLOT(ObjectSelected()));
    connect(ui->find, SIGNAL(clicked()), this, SLOT(Find()));
}

ObjectSelectorWindow::~ObjectSelectorWindow()
{
    delete ui;
}

void ObjectSelectorWindow::Bind(ObjectSelectorList* listControl, bool findRef)
{
    auto existingControls = findChildren<ObjectSelectorList*>();
    for(auto ctrl : existingControls)
    {
        ui->listContainerLayout->removeWidget(ctrl);

        ctrl->deleteLater();
    }

    if(findRef)
    {
        ui->find->show();
    }

    ui->listContainerLayout->addWidget(listControl);

    connect(listControl, SIGNAL(selectedObjectChanged()), this,
        SLOT(SelectedObjectChanged()));
}

void ObjectSelectorWindow::Open(ObjectSelectorBase* ctrl)
{
    mSelectorControl = ctrl;

    uint32_t value = 0;
    if(ctrl)
    {
        value = ctrl->GetValue();
        ui->select->show();
    }
    else
    {
        ui->select->hide();
    }

    // Load object list if needed
    auto listControl = findChild<ObjectSelectorList*>();
    if(listControl)
    {
        listControl->LoadIfNeeded();

        if(value)
        {
            listControl->Select(value);
        }
    }

    show();
    raise();
}

bool ObjectSelectorWindow::CloseIfConnected(QWidget* topLevel)
{
    if(isVisible() && mSelectorControl)
    {
        QObject* parent = mSelectorControl;
        while(parent)
        {
            if(parent == topLevel)
            {
                mSelectorControl = nullptr;
                close();
                return true;
            }

            parent = parent->parent();
        }
    }

    return false;
}

void ObjectSelectorWindow::closeEvent(QCloseEvent* event)
{
    if(mFindWindow)
    {
        if(!mFindWindow->close())
        {
            // Close failed (possibly searching) so ignore
            event->ignore();
            return;
        }

        mFindWindow->deleteLater();
        mFindWindow = 0;
    }
}

void ObjectSelectorWindow::ObjectSelected()
{
    auto listControl = findChild<ObjectSelectorList*>();
    if(listControl && mSelectorControl)
    {
        auto obj = listControl->GetSelectedObject();
        if(obj)
        {
            mSelectorControl->SetValue((uint32_t)listControl
                ->GetObjectID(obj).toUInt());
            mSelectorControl = nullptr;
            close();
        }
    }
}

void ObjectSelectorWindow::SelectedObjectChanged()
{
    auto listControl = findChild<ObjectSelectorList*>();
    if(listControl)
    {
        auto obj = listControl->GetSelectedObject();
        if(obj)
        {
            ui->select->setDisabled(false);
        }
        else
        {
            ui->select->setDisabled(true);
        }
    }
}

void ObjectSelectorWindow::Find()
{
    auto listControl = findChild<ObjectSelectorList*>();
    if(listControl)
    {
        if(!mFindWindow)
        {
            mFindWindow = new FindRefWindow(mMainWindow);
        }
        
        uint32_t val = 0;
        auto obj = listControl->GetSelectedObject();
        if(obj)
        {
            val = (uint32_t)listControl->GetObjectID(obj).toUInt();
        }

        mFindWindow->Open(listControl->GetObjectType(), val);
    }
}
