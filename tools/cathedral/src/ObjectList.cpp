/**
 * @file tools/cathedral/src/ObjectList.h
 * @ingroup cathedral
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Implementation for a list that holds objgen objects.
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

#include "ObjectList.h"

// Cathedral Includes
#include "MainWindow.h"
#include "ObjectListModel.h"

// Qt Includes
#include <PopIgnore.h>
#include <PushIgnore.h>

#include <QSortFilterProxyModel>

#include "ui_ObjectList.h"

// libcomp Includes
#include <Log.h>

ObjectList::ObjectList(QWidget* pParent)
    : QWidget(pParent), mMainWindow(nullptr), mReadOnly(false) {
  mObjectModel = new ObjectListModel(this);

  mFilterModel = new QSortFilterProxyModel;
  mFilterModel->setSourceModel(mObjectModel);
  mFilterModel->setFilterRegExp(
      QRegExp("", Qt::CaseInsensitive, QRegExp::FixedString));
  mFilterModel->setFilterKeyColumn(0);

  ui = new Ui::ObjectList;
  ui->setupUi(this);

  ui->objectList->setModel(mFilterModel);

  ui->moveUp->setHidden(true);
  ui->moveDown->setHidden(true);
  ui->moveUp->setDisabled(true);
  ui->moveDown->setDisabled(true);

  connect(ui->objectSearch, SIGNAL(textChanged(const QString&)), this,
          SLOT(Search(const QString&)));
  connect(
      ui->objectList->selectionModel(),
      SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
      this, SLOT(SelectedObjectChanged()));

  connect(ui->moveUp, SIGNAL(clicked(bool)), this, SLOT(MoveUp()));
  connect(ui->moveDown, SIGNAL(clicked(bool)), this, SLOT(MoveDown()));
}

ObjectList::~ObjectList() { delete ui; }

void ObjectList::SetMainWindow(MainWindow* pMainWindow) {
  mMainWindow = pMainWindow;
}

void ObjectList::Search(const QString& term) {
  mFilterModel->setFilterRegExp(
      QRegExp(term, Qt::CaseInsensitive, QRegExp::FixedString));
}

QString ObjectList::GetObjectID(
    const std::shared_ptr<libcomp::Object>& obj) const {
  (void)obj;

  return {};
}

QString ObjectList::GetObjectName(
    const std::shared_ptr<libcomp::Object>& obj) const {
  (void)obj;

  return {};
}

bool ObjectList::Select(const std::shared_ptr<libcomp::Object>& obj) {
  int idx = mObjectModel->GetIndex(obj);
  if (idx != -1) {
    auto qIdx = mObjectModel->index(idx);

    ui->objectList->scrollTo(qIdx,
                             QAbstractItemView::ScrollHint::PositionAtCenter);
    ui->objectList->selectionModel()->select(
        qIdx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

    return true;
  }

  return false;
}

void ObjectList::SetObjectList(
    const std::vector<std::shared_ptr<libcomp::Object>>& objs) {
  mObjectModel->SetObjectList(objs);

  // Always reset to no selection
  mActiveObject.reset();
  LoadProperties(nullptr);
}

void ObjectList::LoadProperties(const std::shared_ptr<libcomp::Object>& obj) {
  (void)obj;
}

void ObjectList::SaveProperties(const std::shared_ptr<libcomp::Object>& obj) {
  (void)obj;
}

std::shared_ptr<libcomp::Object> ObjectList::GetActiveObject() {
  return mActiveObject.lock();
}

void ObjectList::SaveActiveProperties() {
  if (!mReadOnly) {
    auto obj = mActiveObject.lock();
    if (obj) {
      SaveProperties(obj);
    }
  }
}

void ObjectList::SelectedObjectChanged() {
  auto obj = mActiveObject.lock();

  if (obj) {
    if (mMainWindow) {
      mMainWindow->CloseSelectors(this);
    }

    if (!mReadOnly) {
      SaveProperties(obj);
    }
  }

  auto idxList = ui->objectList->selectionModel()->selectedIndexes();

  if (idxList.isEmpty()) {
    mActiveObject = {};

    ui->moveUp->setDisabled(true);
    ui->moveDown->setDisabled(true);
  } else {
    mActiveObject =
        mObjectModel->GetObject(mFilterModel->mapToSource(idxList.at(0)));

    ui->moveUp->setDisabled(false);
    ui->moveDown->setDisabled(false);
  }

  LoadProperties(mActiveObject.lock());

  emit selectedObjectChanged();
}

void ObjectList::MoveUp() {
  auto idxList = ui->objectList->selectionModel()->selectedIndexes();
  if (!idxList.isEmpty() && idxList.at(0).row() != 0) {
    auto obj =
        mObjectModel->GetObject(mFilterModel->mapToSource(idxList.at(0)));
    emit objectMoved(obj, true);
  }
}

void ObjectList::MoveDown() {
  auto idxList = ui->objectList->selectionModel()->selectedIndexes();
  if (!idxList.isEmpty() &&
      idxList.at(0).row() != mObjectModel->rowCount() - 1) {
    auto obj =
        mObjectModel->GetObject(mFilterModel->mapToSource(idxList.at(0)));
    emit objectMoved(obj, false);
  }
}

std::map<uint32_t, QString> ObjectList::GetObjectMapping() const {
  std::map<uint32_t, QString> mapping;

  int count = mObjectModel->rowCount();

  for (int i = 0; i < count; ++i) {
    auto idx = mObjectModel->index(i);
    auto obj = mObjectModel->GetObject(idx);

    mapping[GetObjectID(obj).toUInt()] = mObjectModel->data(idx).toString();
  }

  return mapping;
}

void ObjectList::SetReadOnly(bool readOnly) { mReadOnly = readOnly; }

void ObjectList::ToggleMoveControls(bool visible) {
  ui->moveUp->setHidden(!visible);
  ui->moveDown->setHidden(!visible);
}
