/**
 * @file tools/capgrep/src/SearchFilter.cpp
 * @ingroup capgrep
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Packet list filter to only show matching packets.
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

#include "SearchFilter.h"

#include <Convert.h>
#include <PopIgnore.h>
#include <PushIgnore.h>

#include <QSettings>

#include "PacketData.h"
#include "PacketListFilter.h"
#include "PacketListModel.h"

SearchFilter::SearchFilter(QObject* p)
    : QSortFilterProxyModel(p), mSearchType(SearchType_None), mCommand(0) {}

bool SearchFilter::filterAcceptsRow(int row, const QModelIndex& p) const {
  Q_UNUSED(p)

  PacketListFilter* filter = qobject_cast<PacketListFilter*>(sourceModel());
  if (!filter) return false;

  PacketListModel* model =
      qobject_cast<PacketListModel*>(filter->sourceModel());
  if (!model) return false;

  PacketData* d = model->packetAt(filter->mapRow(row));
  if (!d) return false;

  switch (mSearchType) {
    case SearchType_Binary:
    case SearchType_Text:
      return d->data.contains(mTerm);
    case SearchType_Command:
      return d->cmd == mCommand;
    case SearchType_None:
    default:
      break;
  }

  return false;
}

void SearchFilter::reset() {
  mSearchType = SearchType_None;
  mTerm.clear();
  mCommand = 0;

  beginResetModel();
  invalidateFilter();
  endResetModel();
}

void SearchFilter::findBinary(const QByteArray& term) {
  mSearchType = SearchType_Binary;
  mTerm = term;

  invalidateFilter();
}

void SearchFilter::findText(const QString& encoding, const QString& text) {
  mSearchType = SearchType_Text;
  if ("CP1252" == encoding) {
    std::vector<char> term = libcomp::Convert::ToEncoding(
        libcomp::Convert::ENCODING_CP1252, text.toUtf8().constData());
    mTerm = QByteArray(&term[0], static_cast<int>(term.size()));
  } else if ("CP932" == encoding) {
    std::vector<char> term = libcomp::Convert::ToEncoding(
        libcomp::Convert::ENCODING_CP932, text.toUtf8().constData());
    mTerm = QByteArray(&term[0], static_cast<int>(term.size()));
  } else {
    mTerm = text.toUtf8();
  }
  mTerm.chop(1);

  invalidateFilter();
}

void SearchFilter::findCommand(uint16_t cmd) {
  mSearchType = SearchType_Command;
  mCommand = cmd;

  invalidateFilter();
}

bool SearchFilter::searchResult(const QModelIndex& idx, int& packet,
                                int& offset, QByteArray& term) {
  PacketListFilter* filter = qobject_cast<PacketListFilter*>(sourceModel());
  if (!filter) return false;

  PacketListModel* model =
      qobject_cast<PacketListModel*>(filter->sourceModel());
  if (!model) return false;

  packet = filter->mapToSource(mapToSource(idx)).row();

  PacketData* d = model->packetAt(packet);
  if (!d) return false;

  if (mSearchType == SearchType_Command) {
    term.clear();
    offset = -1;
  } else {
    term = mTerm;
    offset = d->data.indexOf(mTerm);
  }

  return true;
}
