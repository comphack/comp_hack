/**
 * @file tools/cathedral/src/ObjectList.h
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Definition for a list of objgen objects.
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

#ifndef TOOLS_CATHEDRAL_SRC_OBJECTLIST_H
#define TOOLS_CATHEDRAL_SRC_OBJECTLIST_H

// Ignore warnings
#include <PushIgnore.h>

// Qt Includes
#include <QWidget>

// Stop ignoring warnings
#include <PopIgnore.h>

// objects Includes
#include <Object.h>

namespace Ui {

class ObjectList;

}  // namespace Ui

class MainWindow;
class ObjectListModel;
class QSortFilterProxyModel;

class ObjectList : public QWidget {
  Q_OBJECT

 public:
  explicit ObjectList(QWidget* pParent = 0);
  virtual ~ObjectList();

  virtual void SetMainWindow(MainWindow* pMainWindow);

  virtual void SetObjectList(
      const std::vector<std::shared_ptr<libcomp::Object>>& objs);

  virtual QString GetObjectID(
      const std::shared_ptr<libcomp::Object>& obj) const = 0;
  virtual QString GetObjectName(
      const std::shared_ptr<libcomp::Object>& obj) const;

  bool Select(const std::shared_ptr<libcomp::Object>& obj);

  virtual void LoadProperties(const std::shared_ptr<libcomp::Object>& obj);
  virtual void SaveProperties(const std::shared_ptr<libcomp::Object>& obj);

  std::shared_ptr<libcomp::Object> GetActiveObject();

  void SaveActiveProperties();

  std::map<uint32_t, QString> GetObjectMapping() const;

  void SetReadOnly(bool readOnly);

  void ToggleMoveControls(bool visible);

  template <class T>
  static bool Move(std::list<std::shared_ptr<T>>& list, std::shared_ptr<T> obj,
                   bool up) {
    auto iter = list.begin();
    auto iter2 = list.end();
    if (!up) {
      iter2 = list.begin();
      iter2++;
    }

    while (iter != list.end() && (up || iter2 != list.end())) {
      if (*iter == obj) {
        if (up) {
          // Make sure its not already at the top
          if (iter2 != list.end()) {
            list.splice(iter2, list, iter);
          }
        } else {
          list.splice(iter, list, iter2);
        }
        return true;
      }

      iter++;

      if (up && iter2 == list.end()) {
        iter2 = list.begin();
      } else {
        iter2++;
      }
    }

    return false;
  }

 signals:
  void selectedObjectChanged();
  void objectMoved(std::shared_ptr<libcomp::Object> obj, bool up);

 public slots:
  virtual void Search(const QString& term);
  virtual void SelectedObjectChanged();
  void MoveUp();
  void MoveDown();

 protected:
  MainWindow* mMainWindow;
  ObjectListModel* mObjectModel;
  QSortFilterProxyModel* mFilterModel;

  std::weak_ptr<libcomp::Object> mActiveObject;

  Ui::ObjectList* ui;

  bool mReadOnly;
};

#endif  // TOOLS_CATHEDRAL_SRC_OBJECTLIST_H
