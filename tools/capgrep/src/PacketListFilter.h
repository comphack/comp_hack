/**
 * @file tools/capgrep/src/PacketListFilter.h
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

#ifndef TOOLS_CAPGREP_SRC_PACKETFILTERLIST_H
#define TOOLS_CAPGREP_SRC_PACKETFILTERLIST_H

// Standard C Includes
#include <stdint.h>

// Ignore warnings
#include <PushIgnore.h>

#include <QSortFilterProxyModel>

// Stop ignoring warnings
#include <PopIgnore.h>

class PacketListFilter : public QSortFilterProxyModel {
  Q_OBJECT

 public:
  PacketListFilter(QObject* parent = 0);

  QList<uint16_t> white() const;
  QList<uint16_t> black() const;

  void addWhite(uint16_t cmd);
  void addBlack(uint16_t cmd);
  void removeWhite(uint16_t cmd);
  void removeBlack(uint16_t cmd);

  void clear();
  void clearWhite();
  void clearBlack();
  void reset();

  void setWhite(const QList<uint16_t>& cmds);
  void setBlack(const QList<uint16_t>& cmds);
  void setFilter(const QList<uint16_t>& w, const QList<uint16_t>& b);

  int mapRow(int row) const;

 protected:
  void saveBoth();
  void saveWhite();
  void saveBlack();

  bool filterAcceptsRow(int row, const QModelIndex& parent) const;

  QList<uint16_t> mWhiteList, mBlackList;
};

#endif  // TOOLS_CAPGREP_SRC_PACKETFILTERLIST_H
