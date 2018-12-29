/**
 * @file tools/cathedral/src/ObjectList.h
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Definition for a list of objgen objects.
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

#ifndef TOOLS_CATHEDRAL_SRC_OBJECTLIST_H
#define TOOLS_CATHEDRAL_SRC_OBJECTLIST_H

// Qt Includes
#include <PushIgnore.h>
#include <QWidget>
#include <PopIgnore.h>

// objects Includes
#include <Object.h>

namespace Ui
{

class ObjectList;

} // namespace Ui

class MainWindow;
class ObjectListModel;
class QSortFilterProxyModel;

class ObjectList : public QWidget
{
    Q_OBJECT

public:
    explicit ObjectList(QWidget *pParent = 0);
    virtual ~ObjectList();

    virtual void SetMainWindow(MainWindow *pMainWindow);

    virtual void SetObjectList(const std::vector<
        std::shared_ptr<libcomp::Object>>& objs);

    virtual QString GetObjectID(const std::shared_ptr<
        libcomp::Object>& obj) const = 0;
    virtual QString GetObjectName(const std::shared_ptr<
        libcomp::Object>& obj) const;

    bool Select(const std::shared_ptr<libcomp::Object>& obj);

    virtual void LoadProperties(const std::shared_ptr<libcomp::Object>& obj);
    virtual void SaveProperties(const std::shared_ptr<libcomp::Object>& obj);

    void SaveActiveProperties();

    std::map<uint32_t, QString> GetObjectMapping() const;

public slots:
    virtual void Search(const QString& term);
    virtual void SelectedObjectChanged();

protected:
    MainWindow *mMainWindow;
    ObjectListModel *mObjectModel;
    QSortFilterProxyModel *mFilterModel;

    std::weak_ptr<libcomp::Object> mActiveObject;

    Ui::ObjectList *ui;
};

#endif // TOOLS_CATHEDRAL_SRC_OBJECTLIST_H
