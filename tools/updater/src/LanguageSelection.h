/**
 * @file tools/updater/src/LanguageSelection.h
 * @ingroup tools
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief GUI for the language selection.
 *
 * This tool will update the game client.
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

#ifndef TOOLS_UPDATER_SRC_LANGUAGESELECTION_H
#define TOOLS_UPDATER_SRC_LANGUAGESELECTION_H

#include <PopIgnore.h>
#include <PushIgnore.h>
#include <d3d9.h>

#include <QDialog>

#include "ui_LanguageSelection.h"

class LanguageSelection : public QDialog {
  Q_OBJECT

 public:
  LanguageSelection(QWidget *parent = 0);
  ~LanguageSelection();

 protected slots:
  void LanguageChanged();
  void Save();

 protected:
  virtual void changeEvent(QEvent *pEvent);

  Ui::LanguageSelection ui;
};

#endif  // TOOLS_UPDATER_SRC_LANGUAGESELECTION_H
