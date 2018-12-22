/**
 * @file tools/cathedral/src/DynamicList.h
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Definition for a list of controls of a dynamic type.
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

#ifndef TOOLS_CATHEDRAL_SRC_DYNAMICLIST_H
#define TOOLS_CATHEDRAL_SRC_DYNAMICLIST_H

// Qt Includes
#include <PushIgnore.h>
#include <QWidget>
#include <PopIgnore.h>

// libcomp Includes
#include <CString.h>

// Standard C++11 Includes
#include <list>
#include <memory>

namespace libcomp
{

class Object;

} // namespace libcomp

namespace Ui
{

class DynamicList;

} // namespace Ui

class MainWindow;

enum class DynamicItemType_t : uint8_t
{
    NONE,
    PRIMITIVE_INT,
    PRIMITIVE_UINT,
    PRIMITIVE_STRING,
    COMPLEX_EVENT_MESSAGE,
    OBJ_EVENT_BASE,
    OBJ_EVENT_CHOICE,
    OBJ_EVENT_CONDITION,
    OBJ_ITEM_DROP,
    OBJ_OBJECT_POSITION,
};

class DynamicList : public QWidget
{
    Q_OBJECT

public:
    explicit DynamicList(QWidget *pParent = 0);
    virtual ~DynamicList();

    void Setup(DynamicItemType_t type, MainWindow *pMainWindow);

    bool AddInteger(int32_t val);
    bool AddUnsignedInteger(uint32_t val);
    bool AddString(const libcomp::String& val);
    template<class T> bool AddObject(const std::shared_ptr<T>& obj);

    std::list<int32_t> GetIntegerList() const;
    std::list<uint32_t> GetUnsignedIntegerList() const;
    std::list<libcomp::String> GetStringList() const;
    template<class T> std::list<std::shared_ptr<T>> GetObjectList() const;

protected slots:
    void AddRow();
    void RemoveRow();
    void MoveUp();
    void MoveDown();

signals:
    void rowEdit();

protected:
    void AddItem(QWidget* ctrl, bool canReorder);
    QWidget* GetIntegerWidget(int32_t val);
    QWidget* GetUnsignedIntegerWidget(uint32_t val);
    QWidget* GetStringWidget(const libcomp::String& val);
    template<class T> QWidget* GetObjectWidget(const std::shared_ptr<T>& obj);
    QWidget* GetEventMessageWidget(int32_t val);

    void RefreshPositions();

    Ui::DynamicList *ui;

    MainWindow *mMainWindow;

    DynamicItemType_t mType;
};

#endif // TOOLS_CATHEDRAL_SRC_DYNAMICLIST_H
