/**
 * @file tools/cathedral/src/ObjectSelectorWindow.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for a value selection window bound to an
 *  ObjectSelector.
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

#include "ObjectSelectorWindow.h"

// Cathedral Includes
#include "MainWindow.h"
#include "ObjectSelectorBase.h"
#include "ObjectSelectorList.h"

// Qt Includes
#include <PushIgnore.h>
#include "ui_ObjectSelectorWindow.h"
#include <PopIgnore.h>

ObjectSelectorWindow::ObjectSelectorWindow(QWidget *pParent) :
    QWidget(pParent), mSelectorControl(0)
{
    ui = new Ui::ObjectSelectorWindow;
    ui->setupUi(this);

    ui->select->setDisabled(true);

    connect(ui->select, SIGNAL(clicked()), this, SLOT(ObjectSelected()));
}

ObjectSelectorWindow::~ObjectSelectorWindow()
{
    delete ui;
}

void ObjectSelectorWindow::Bind(ObjectSelectorList* listControl)
{
    auto existingControls = findChildren<ObjectSelectorList*>();
    for(auto ctrl : existingControls)
    {
        ui->listContainerLayout->removeWidget(ctrl);

        ctrl->deleteLater();
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
    auto item = ui->listContainerLayout->itemAt(0);
    if(item)
    {
        auto list = (ObjectSelectorList*)item->widget();
        list->LoadIfNeeded();

        if(value)
        {
            list->Select(value);
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

void ObjectSelectorWindow::ObjectSelected()
{
    auto item = ui->listContainerLayout->itemAt(0);
    if(item && mSelectorControl)
    {
        auto list = (ObjectSelectorList*)item->widget();

        auto obj = list->GetSelectedObject();
        if(obj)
        {
            mSelectorControl->SetValue((uint32_t)list
                ->GetObjectID(obj).toUInt());
            mSelectorControl = nullptr;
            close();
        }
    }
}

void ObjectSelectorWindow::SelectedObjectChanged()
{
    auto item = ui->listContainerLayout->itemAt(0);
    if(item)
    {
        auto list = (ObjectSelectorList*)item->widget();

        auto obj = list->GetSelectedObject();
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
