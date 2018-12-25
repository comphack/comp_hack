/**
 * @file tools/cathedral/src/DynamicList.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for a list of controls of a dynamic type.
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

#include "DynamicList.h"

// cathedral Includes
#include "DynamicListItem.h"
#include "EventBaseUI.h"
#include "EventChoiceUI.h"
#include "EventConditionUI.h"
#include "EventMessageRef.h"
#include "ItemDropUI.h"
#include "MainWindow.h"
#include "ObjectPositionUI.h"
#include "ObjectSelector.h"

// Qt Includes
#include <PushIgnore.h>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QLineEdit>

#include "ui_DynamicList.h"
#include "ui_DynamicListItem.h"
#include <PopIgnore.h>

// objects Includes
#include <EventBase.h>
#include <EventChoice.h>
#include <EventCondition.h>
#include <ItemDrop.h>
#include <ObjectPosition.h>

// libcomp Includes
#include <Log.h>
#include <Object.h>

DynamicList::DynamicList(QWidget *pParent) : QWidget(pParent),
    mType(DynamicItemType_t::NONE)
{
    ui = new Ui::DynamicList;
    ui->setupUi(this);

    connect(ui->add, SIGNAL(clicked(bool)), this, SLOT(AddRow()));
}

DynamicList::~DynamicList()
{
    delete ui;
}

void DynamicList::Setup(DynamicItemType_t type, MainWindow *pMainWindow,
    const libcomp::String& objectSelectorType)
{
    if(mType == DynamicItemType_t::NONE)
    {
        mType = type;
        mMainWindow = pMainWindow;

        if(type == DynamicItemType_t::COMPLEX_OBJECT_SELECTOR)
        {
            mObjectSelectorType = objectSelectorType;
        }
    }
    else
    {
        LOG_ERROR("Attempted to set a DynamicList item type twice\n");
    }
}

bool DynamicList::AddInteger(int32_t val)
{
    if(mType == DynamicItemType_t::COMPLEX_EVENT_MESSAGE)
    {
        auto ctrl = GetEventMessageWidget(val);
        if(ctrl)
        {
            AddItem(ctrl, true);
            return true;
        }
    }
    else
    {
        if(mType != DynamicItemType_t::PRIMITIVE_INT)
        {
            LOG_ERROR("Attempted to assign a signed integer value to a differing"
                " DynamicList type\n");
            return false;
        }

        auto ctrl = GetIntegerWidget(val);
        if(ctrl)
        {
            AddItem(ctrl, false);
            return true;
        }
    }

    return false;
}

QWidget* DynamicList::GetIntegerWidget(int32_t val)
{
    QSpinBox* spin = new QSpinBox;
    spin->setMaximum(2147483647);
    spin->setMinimum(-2147483647);

    spin->setValue(val);

    return spin;
}

bool DynamicList::AddUnsignedInteger(uint32_t val)
{
    if(mType == DynamicItemType_t::COMPLEX_OBJECT_SELECTOR)
    {
        auto ctrl = GetObjectSelectorWidget(val);
        if(ctrl)
        {
            AddItem(ctrl, true);
            return true;
        }
    }
    else
    {
        if(mType != DynamicItemType_t::PRIMITIVE_UINT)
        {
            LOG_ERROR("Attempted to assign a unsigned integer value to a"
                " differing DynamicList type\n");
            return false;
        }

        auto ctrl = GetUnsignedIntegerWidget(val);
        if(ctrl)
        {
            AddItem(ctrl, false);
            return true;
        }
    }

    return false;
}

QWidget* DynamicList::GetUnsignedIntegerWidget(uint32_t val)
{
    QSpinBox* spin = new QSpinBox;
    spin->setMaximum(2147483647);
    spin->setMinimum(0);

    spin->setValue((int32_t)val);

    return spin;
}

bool DynamicList::AddString(const libcomp::String& val)
{
    if(mType != DynamicItemType_t::PRIMITIVE_STRING)
    {
        LOG_ERROR("Attempted to assign a string value to a differing"
            " DynamicList type\n");
        return false;
    }

    auto ctrl = GetStringWidget(val);
    if(ctrl)
    {
        AddItem(ctrl, false);
        return true;
    }

    return false;
}

QWidget* DynamicList::GetStringWidget(const libcomp::String& val)
{
    QLineEdit* txt = new QLineEdit;
    txt->setPlaceholderText("[Empty]");

    txt->setText(qs(val));

    return txt;
}

template<>
QWidget* DynamicList::GetObjectWidget<objects::EventBase>(
    const std::shared_ptr<objects::EventBase>& obj)
{
    EventBase* ctrl = new EventBase(mMainWindow);
    if(obj)
    {
        ctrl->Load(obj);
    }

    return ctrl;
}

template<>
bool DynamicList::AddObject<objects::EventBase>(
    const std::shared_ptr<objects::EventBase>& obj)
{
    if(mType != DynamicItemType_t::OBJ_EVENT_BASE)
    {
        LOG_ERROR("Attempted to assign an EventBase object to a differing"
            " DynamicList type\n");
        return false;
    }

    auto ctrl = GetObjectWidget(obj);
    if(ctrl)
    {
        AddItem(ctrl, true);
        return true;
    }

    return false;
}

template<>
QWidget* DynamicList::GetObjectWidget<objects::EventChoice>(
    const std::shared_ptr<objects::EventChoice>& obj)
{
    EventChoice* ctrl = new EventChoice(mMainWindow);
    if(obj)
    {
        ctrl->Load(obj);
    }

    return ctrl;
}

template<>
bool DynamicList::AddObject<objects::EventChoice>(
    const std::shared_ptr<objects::EventChoice>& obj)
{
    if(mType != DynamicItemType_t::OBJ_EVENT_CHOICE)
    {
        LOG_ERROR("Attempted to assign an EventChoice object to a differing"
            " DynamicList type\n");
        return false;
    }

    auto ctrl = GetObjectWidget(obj);
    if(ctrl)
    {
        AddItem(ctrl, true);
        return true;
    }

    return false;
}

template<>
QWidget* DynamicList::GetObjectWidget<objects::EventCondition>(
    const std::shared_ptr<objects::EventCondition>& obj)
{
    EventCondition* ctrl = new EventCondition(mMainWindow);
    if(obj)
    {
        ctrl->Load(obj);
    }

    return ctrl;
}

template<>
bool DynamicList::AddObject<objects::EventCondition>(
    const std::shared_ptr<objects::EventCondition>& obj)
{
    if(mType != DynamicItemType_t::OBJ_EVENT_CONDITION)
    {
        LOG_ERROR("Attempted to assign an EventCondition object to a differing"
            " DynamicList type\n");
        return false;
    }

    auto ctrl = GetObjectWidget(obj);
    if(ctrl)
    {
        AddItem(ctrl, true);
        return true;
    }

    return false;
}

template<>
QWidget* DynamicList::GetObjectWidget<objects::ItemDrop>(
    const std::shared_ptr<objects::ItemDrop>& obj)
{
    ItemDrop* ctrl = new ItemDrop(mMainWindow);
    if(obj)
    {
        ctrl->Load(obj);
    }

    return ctrl;
}

template<>
bool DynamicList::AddObject<objects::ItemDrop>(
    const std::shared_ptr<objects::ItemDrop>& obj)
{
    if(mType != DynamicItemType_t::OBJ_ITEM_DROP)
    {
        LOG_ERROR("Attempted to assign an ItemDrop object to a differing"
            " DynamicList type\n");
        return false;
    }

    auto ctrl = GetObjectWidget(obj);
    if(ctrl)
    {
        AddItem(ctrl, true);
        return true;
    }

    return false;
}

template<>
QWidget* DynamicList::GetObjectWidget<objects::ObjectPosition>(
    const std::shared_ptr<objects::ObjectPosition>& obj)
{
    ObjectPosition* ctrl = new ObjectPosition;
    if(obj)
    {
        ctrl->Load(obj);
    }

    return ctrl;
}

template<>
bool DynamicList::AddObject<objects::ObjectPosition>(
    const std::shared_ptr<objects::ObjectPosition>& obj)
{
    if(mType != DynamicItemType_t::OBJ_OBJECT_POSITION)
    {
        LOG_ERROR("Attempted to assign an ObjectPosition object to a differing"
            " DynamicList type\n");
        return false;
    }

    auto ctrl = GetObjectWidget(obj);
    if(ctrl)
    {
        AddItem(ctrl, true);
        return true;
    }

    return false;
}

QWidget* DynamicList::GetEventMessageWidget(int32_t val)
{
    EventMessageRef* msg = new EventMessageRef;
    msg->SetMainWindow(mMainWindow);

    msg->SetValue(val);

    return msg;
}

QWidget* DynamicList::GetObjectSelectorWidget(uint32_t val)
{
    ObjectSelector* sel = new ObjectSelector;
    sel->Bind(mMainWindow, mObjectSelectorType);

    sel->SetValue(val);

    return sel;
}

std::list<int32_t> DynamicList::GetIntegerList() const
{
    std::list<int32_t> result;
    if(mType == DynamicItemType_t::COMPLEX_EVENT_MESSAGE)
    {
        int total = ui->layoutItems->count();
        for(int childIdx = 0; childIdx < total; childIdx++)
        {
            EventMessageRef* ref = ui->layoutItems->itemAt(childIdx)->widget()
                ->findChild<EventMessageRef*>();
            result.push_back(ref->GetValue());
        }
    }
    else
    {
        if(mType != DynamicItemType_t::PRIMITIVE_INT)
        {
            LOG_ERROR("Attempted to retrieve signed integer list from a"
                " differing DynamicList type\n");
            return result;
        }
    
        int total = ui->layoutItems->count();
        for(int childIdx = 0; childIdx < total; childIdx++)
        {
            QSpinBox* spin = ui->layoutItems->itemAt(childIdx)->widget()
                ->findChild<QSpinBox*>();
            result.push_back((int32_t)spin->value());
        }
    }

    return result;
}

std::list<uint32_t> DynamicList::GetUnsignedIntegerList() const
{
    std::list<uint32_t> result;
    if(mType == DynamicItemType_t::COMPLEX_OBJECT_SELECTOR)
    {
        int total = ui->layoutItems->count();
        for(int childIdx = 0; childIdx < total; childIdx++)
        {
            ObjectSelector* sel = ui->layoutItems->itemAt(childIdx)->widget()
                ->findChild<ObjectSelector*>();
            result.push_back(sel->GetValue());
        }
    }
    else
    {
        if(mType != DynamicItemType_t::PRIMITIVE_UINT)
        {
            LOG_ERROR("Attempted to retrieve unsigned integer list from a"
                " differing DynamicList type\n");
            return result;
        }

        int total = ui->layoutItems->count();
        for(int childIdx = 0; childIdx < total; childIdx++)
        {
            QSpinBox* spin = ui->layoutItems->itemAt(childIdx)->widget()
                ->findChild<QSpinBox*>();
            result.push_back((uint32_t)spin->value());
        }
    }

    return result;
}

std::list<libcomp::String> DynamicList::GetStringList() const
{
    std::list<libcomp::String> result;
    if(mType != DynamicItemType_t::PRIMITIVE_STRING)
    {
        LOG_ERROR("Attempted to retrieve a string list from a differing"
            " DynamicList type\n");
        return result;
    }
    
    int total = ui->layoutItems->count();
    for(int childIdx = 0; childIdx < total; childIdx++)
    {
        QLineEdit* txt = ui->layoutItems->itemAt(childIdx)->widget()
            ->findChild<QLineEdit*>();
        result.push_back(libcomp::String(txt->text().toStdString()));
    }

    return result;
}

template<>
std::list<std::shared_ptr<objects::EventBase>>
    DynamicList::GetObjectList() const
{
    std::list<std::shared_ptr<objects::EventBase>> result;
    if(mType != DynamicItemType_t::OBJ_EVENT_BASE)
    {
        LOG_ERROR("Attempted to retrieve an EventBase list from a differing"
            " DynamicList type\n");
        return result;
    }

    int total = ui->layoutItems->count();
    for(int childIdx = 0; childIdx < total; childIdx++)
    {
        EventBase* ctrl = ui->layoutItems->itemAt(childIdx)->widget()
            ->findChild<EventBase*>();
        result.push_back(ctrl->Save());
    }

    return result;
}

template<>
std::list<std::shared_ptr<objects::EventChoice>>
    DynamicList::GetObjectList() const
{
    std::list<std::shared_ptr<objects::EventChoice>> result;
    if(mType != DynamicItemType_t::OBJ_EVENT_CHOICE)
    {
        LOG_ERROR("Attempted to retrieve an EventChoice list from a differing"
            " DynamicList type\n");
        return result;
    }

    int total = ui->layoutItems->count();
    for(int childIdx = 0; childIdx < total; childIdx++)
    {
        EventChoice* ctrl = ui->layoutItems->itemAt(childIdx)->widget()
            ->findChild<EventChoice*>();
        result.push_back(ctrl->Save());
    }

    return result;
}

template<>
std::list<std::shared_ptr<objects::EventCondition>>
    DynamicList::GetObjectList() const
{
    std::list<std::shared_ptr<objects::EventCondition>> result;
    if(mType != DynamicItemType_t::OBJ_EVENT_CONDITION)
    {
        LOG_ERROR("Attempted to retrieve an EventCondition list from a"
            " differing DynamicList type\n");
        return result;
    }

    int total = ui->layoutItems->count();
    for(int childIdx = 0; childIdx < total; childIdx++)
    {
        EventCondition* ctrl = ui->layoutItems->itemAt(childIdx)->widget()
            ->findChild<EventCondition*>();
        result.push_back(ctrl->Save());
    }

    return result;
}

template<>
std::list<std::shared_ptr<objects::ItemDrop>>
    DynamicList::GetObjectList() const
{
    std::list<std::shared_ptr<objects::ItemDrop>> result;
    if(mType != DynamicItemType_t::OBJ_ITEM_DROP)
    {
        LOG_ERROR("Attempted to retrieve an ItemDrop list from a differing"
            " DynamicList type\n");
        return result;
    }

    int total = ui->layoutItems->count();
    for(int childIdx = 0; childIdx < total; childIdx++)
    {
        ItemDrop* ctrl = ui->layoutItems->itemAt(childIdx)->widget()
            ->findChild<ItemDrop*>();
        result.push_back(ctrl->Save());
    }

    return result;
}

template<>
std::list<std::shared_ptr<objects::ObjectPosition>>
    DynamicList::GetObjectList() const
{
    std::list<std::shared_ptr<objects::ObjectPosition>> result;
    if(mType != DynamicItemType_t::OBJ_ITEM_DROP)
    {
        LOG_ERROR("Attempted to retrieve an ObjectPosition list from a"
            " differing DynamicList type\n");
        return result;
    }

    int total = ui->layoutItems->count();
    for(int childIdx = 0; childIdx < total; childIdx++)
    {
        ObjectPosition* ctrl = ui->layoutItems->itemAt(childIdx)->widget()
            ->findChild<ObjectPosition*>();
        result.push_back(ctrl->Save());
    }

    return result;
}

void DynamicList::AddRow()
{
    bool canReorder = false;

    QWidget* ctrl = nullptr;
    switch(mType)
    {
    case DynamicItemType_t::PRIMITIVE_INT:
        ctrl = GetIntegerWidget(0);
        break;
    case DynamicItemType_t::PRIMITIVE_UINT:
        ctrl = GetUnsignedIntegerWidget(0);
        break;
    case DynamicItemType_t::PRIMITIVE_STRING:
        ctrl = GetStringWidget("");
        break;
    case DynamicItemType_t::COMPLEX_EVENT_MESSAGE:
        ctrl = GetEventMessageWidget(0);
        canReorder = true;
        break;
    case DynamicItemType_t::COMPLEX_OBJECT_SELECTOR:
        ctrl = GetObjectSelectorWidget(0);
        canReorder = true;
        break;
    case DynamicItemType_t::OBJ_EVENT_BASE:
        ctrl = GetObjectWidget(std::make_shared<objects::EventBase>());
        canReorder = true;
        break;
    case DynamicItemType_t::OBJ_EVENT_CHOICE:
        ctrl = GetObjectWidget(std::make_shared<objects::EventChoice>());
        canReorder = true;
        break;
    // Allow the controls below this point to default (the object is created on
    // save anyway)
    case DynamicItemType_t::OBJ_EVENT_CONDITION:
        ctrl = GetObjectWidget<objects::EventCondition>(nullptr);
        canReorder = true;
        break;
    case DynamicItemType_t::OBJ_ITEM_DROP:
        ctrl = GetObjectWidget<objects::ItemDrop>(nullptr);
        canReorder = true;
        break;
    case DynamicItemType_t::OBJ_OBJECT_POSITION:
        ctrl = GetObjectWidget<objects::ObjectPosition>(nullptr);
        canReorder = true;
        break;
    case DynamicItemType_t::NONE:
        LOG_ERROR("Attempted to add a row to a DynamicList with no assigned"
            " item type\n");
        return;
    }

    if(ctrl)
    {
        AddItem(ctrl, canReorder);
    }
}

void DynamicList::AddItem(QWidget* ctrl, bool canReorder)
{
    DynamicListItem* item = new DynamicListItem(this);
    item->ui->layoutBody->addWidget(ctrl);

    if(canReorder)
    {
        connect(item->ui->up, SIGNAL(clicked(bool)), this, SLOT(MoveUp()));
        connect(item->ui->down, SIGNAL(clicked(bool)), this, SLOT(MoveDown()));
    }
    else
    {
        item->ui->down->setVisible(false);
        item->ui->up->setVisible(false);
    }

    ui->layoutItems->addWidget(item);

    connect(item->ui->remove, SIGNAL(clicked(bool)), this, SLOT(RemoveRow()));

    RefreshPositions();

    emit rowEdit();
}

void DynamicList::RemoveRow()
{
    QObject* senderObj = sender();
    QObject* parent = senderObj ? senderObj->parent() : 0;
    if(parent)
    {
        bool exists = false;

        int childIdx = 0;
        int total = ui->layoutItems->count();
        for(; childIdx < total; childIdx++)
        {
            if(ui->layoutItems->itemAt(childIdx)->widget() == parent)
            {
                exists = true;
                break;
            }
        }

        if(exists)
        {
            ui->layoutItems->removeWidget((QWidget*)parent);
            parent->deleteLater();

            RefreshPositions();

            emit rowEdit();
        }
    }
}

void DynamicList::MoveUp()
{
    QObject* senderObj = sender();
    QObject* parent = senderObj ? senderObj->parent() : 0;
    if(parent)
    {
        bool exists = false;

        int childIdx = 0;
        int total = ui->layoutItems->count();
        for(; childIdx < total; childIdx++)
        {
            if(ui->layoutItems->itemAt(childIdx)->widget() == parent)
            {
                exists = true;
                break;
            }
        }

        if(exists)
        {
            ui->layoutItems->removeWidget((QWidget*)parent);
            ui->layoutItems->insertWidget((childIdx - 1), (QWidget*)parent);

            RefreshPositions();

            emit rowEdit();
        }
    }
}

void DynamicList::MoveDown()
{
    QObject* senderObj = sender();
    QObject* parent = senderObj ? senderObj->parent() : 0;
    if(parent)
    {
        bool exists = false;

        int childIdx = 0;
        int total = ui->layoutItems->count();
        for(; childIdx < total; childIdx++)
        {
            if(ui->layoutItems->itemAt(childIdx)->widget() == parent)
            {
                exists = true;
                break;
            }
        }

        if(exists)
        {
            ui->layoutItems->removeWidget((QWidget*)parent);
            ui->layoutItems->insertWidget((childIdx + 1), (QWidget*)parent);

            RefreshPositions();

            emit rowEdit();
        }
    }
}

void DynamicList::RefreshPositions()
{
    int total = ui->layoutItems->count();
    for(int childIdx = 0; childIdx < total; childIdx++)
    {
        auto item = (DynamicListItem*)ui->layoutItems->itemAt(childIdx)
            ->widget();
        item->ui->up->setEnabled(childIdx != 0);
        item->ui->down->setEnabled(childIdx != (total - 1));
    }
}
