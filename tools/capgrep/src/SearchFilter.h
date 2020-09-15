/**
 * @file tools/capgrep/src/SearchFilter.h
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

#ifndef TOOLS_CAPGREP_SRC_SEARCHFILTER_H
#define TOOLS_CAPGREP_SRC_SEARCHFILTER_H

#include <PopIgnore.h>
#include <PushIgnore.h>
#include <stdint.h>

#include <QByteArray>
#include <QSortFilterProxyModel>
#include <QString>

class SearchFilter : public QSortFilterProxyModel {
  Q_OBJECT

 public:
  SearchFilter(QObject* parent = 0);

  void reset();

  typedef enum _SearchType {
    SearchType_None = -1,
    SearchType_Binary = 0,
    SearchType_Text = 1,
    SearchType_Command = 2
  } SearchType;

  void findBinary(const QByteArray& term);
  void findText(const QString& encoding, const QString& text);
  void findCommand(uint16_t cmd);

  bool searchResult(const QModelIndex& index, int& packet, int& offset,
                    QByteArray& term);

 protected:
  bool filterAcceptsRow(int row, const QModelIndex& parent) const;

  SearchType mSearchType;

  QByteArray mTerm;
  uint16_t mCommand;
};

#endif  // TOOLS_CAPGREP_SRC_SEARCHFILTER_H
