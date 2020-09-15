/**
 * @file tools/capgrep/src/PacketListFilter.cpp
 * @ingroup capgrep
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Filter for the packet list.
 *
 * Copyright (C) 2010-2020 COMP_hack Team <compomega@tutanota.com>
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

#include "PacketListFilter.h"

#include <QSettings>

#include "PacketData.h"
#include "PacketListModel.h"

PacketListFilter::PacketListFilter(QObject* p) : QSortFilterProxyModel(p) {
  QSettings settings;

  QVariantList whiteList = settings.value("whiteList").toList();
  QVariantList blackList = settings.value("blackList").toList();

  foreach (QVariant cmd, whiteList)
    mWhiteList.append((uint16_t)cmd.toUInt());

  foreach (QVariant cmd, blackList)
    mBlackList.append((uint16_t)cmd.toUInt());
}

void PacketListFilter::saveBoth() {
  saveWhite();
  saveBlack();
}

void PacketListFilter::saveWhite() {
  QVariantList whiteList;

  foreach (uint16_t cmd, mWhiteList)
    whiteList.append(cmd);

  QSettings settings;
  settings.setValue("whiteList", whiteList);
}

void PacketListFilter::saveBlack() {
  QVariantList blackList;

  foreach (uint16_t cmd, mBlackList)
    blackList.append(cmd);

  QSettings settings;
  settings.setValue("blackList", blackList);
}

bool PacketListFilter::filterAcceptsRow(int row, const QModelIndex& p) const {
  Q_UNUSED(p)

  PacketListModel* model = qobject_cast<PacketListModel*>(sourceModel());
  if (!model) return false;

  PacketData* d = model->packetAt(row);
  if (!d) return false;

  if (!mWhiteList.isEmpty()) return mWhiteList.contains(d->cmd);

  return !mBlackList.contains(d->cmd);
}

QList<uint16_t> PacketListFilter::white() const { return mWhiteList; }

QList<uint16_t> PacketListFilter::black() const { return mBlackList; }

void PacketListFilter::addWhite(uint16_t cmd) {
  if (mWhiteList.contains(cmd)) return;

  mWhiteList.append(cmd);

  invalidateFilter();
  saveWhite();
}

void PacketListFilter::addBlack(uint16_t cmd) {
  if (mBlackList.contains(cmd)) return;

  mBlackList.append(cmd);

  invalidateFilter();
  saveBlack();
}

void PacketListFilter::removeWhite(uint16_t cmd) {
  int idx = mWhiteList.indexOf(cmd);
  if (idx < 0) return;

  mWhiteList.removeAt(idx);

  invalidateFilter();
  saveWhite();
}

void PacketListFilter::removeBlack(uint16_t cmd) {
  int idx = mBlackList.indexOf(cmd);
  if (idx < 0) return;

  mBlackList.removeAt(idx);

  invalidateFilter();
  saveBlack();
}

void PacketListFilter::clear() {
  mWhiteList.clear();
  mBlackList.clear();

  invalidateFilter();
  saveBoth();
}

void PacketListFilter::clearWhite() {
  mWhiteList.clear();

  invalidateFilter();
  saveWhite();
}

void PacketListFilter::clearBlack() {
  mBlackList.clear();

  invalidateFilter();
  saveBlack();
}

void PacketListFilter::setWhite(const QList<uint16_t>& cmds) {
  mWhiteList = cmds;

  invalidateFilter();
  saveWhite();
}

void PacketListFilter::setBlack(const QList<uint16_t>& cmds) {
  mBlackList = cmds;

  invalidateFilter();
  saveBlack();
}

void PacketListFilter::setFilter(const QList<uint16_t>& w,
                                 const QList<uint16_t>& b) {
  mWhiteList = w;
  mBlackList = b;

  invalidateFilter();
  saveBoth();
}

void PacketListFilter::reset() {
  beginResetModel();
  invalidateFilter();
  endResetModel();
}

int PacketListFilter::mapRow(int row) const {
  QModelIndex idx = mapToSource(index(row, 0));

  if (idx.isValid()) return idx.row();

  return -1;
}
