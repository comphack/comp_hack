/**
 * @file tools/cathedral/src/SpawnRestrictionUI.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for a configured SpawnRestriction.
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

#include "SpawnRestrictionUI.h"

// objects Includes
#include <SpawnRestriction.h>

// Ignore warnings
#include <PushIgnore.h>

// UI Includes
#include "ui_SpawnRestriction.h"

// Stop ignoring warnings
#include <PopIgnore.h>

SpawnRestriction::SpawnRestriction(QWidget* pParent) : QWidget(pParent) {
  prop = new Ui::SpawnRestriction;
  prop->setupUi(this);

  prop->time->SetValueName(tr("To:"));
  prop->systemTime->SetValueName(tr("To:"));
  prop->date->SetValueName(tr("To:"));
}

SpawnRestriction::~SpawnRestriction() { delete prop; }

void SpawnRestriction::Load(
    const std::shared_ptr<objects::SpawnRestriction>& restrict) {
  // Gather the button flag controls to set further down
  std::vector<QPushButton*> moonControls;
  for (int i = 0; i < prop->layoutMoonWax->count(); i++) {
    moonControls.push_back(
        (QPushButton*)prop->layoutMoonWax->itemAt(i)->widget());
  }

  for (int i = 0; i < prop->layoutMoonWane->count(); i++) {
    moonControls.push_back(
        (QPushButton*)prop->layoutMoonWane->itemAt(i)->widget());
  }

  std::vector<QPushButton*> dayControls;
  for (int i = 0; i < prop->layoutDay->count(); i++) {
    dayControls.push_back((QPushButton*)prop->layoutDay->itemAt(i)->widget());
  }

  if (!restrict) {
    // Clear settings and quit
    std::unordered_map<uint32_t, int32_t> empty;

    prop->disabled->setChecked(false);
    prop->time->Load(empty);
    prop->systemTime->Load(empty);
    prop->date->Load(empty);

    for (size_t i = 0; i < 16; i++) {
      moonControls[i]->setChecked(false);
    }

    for (size_t i = 0; i < 7; i++) {
      dayControls[i]->setChecked(false);
    }

    return;
  }

  prop->disabled->setChecked(restrict->GetDisabled());

  std::unordered_map<uint32_t, int32_t> time;
  std::unordered_map<uint32_t, int32_t> sysTime;
  std::unordered_map<uint32_t, int32_t> date;

  for (auto& pair : restrict->GetTimeRestriction()) {
    time[(uint32_t)pair.first] = (int32_t)pair.second;
  }

  for (auto& pair : restrict->GetSystemTimeRestriction()) {
    sysTime[(uint32_t)pair.first] = (int32_t)pair.second;
  }

  for (auto& pair : restrict->GetDateRestriction()) {
    date[(uint32_t)pair.first] = (int32_t)pair.second;
  }

  prop->time->Load(time);
  prop->systemTime->Load(sysTime);
  prop->date->Load(date);

  uint16_t moonRestrict = restrict->GetMoonRestriction();
  for (size_t i = 0; i < 16; i++) {
    moonControls[i]->setChecked((moonRestrict & (1 << i)) != 0);
  }

  uint8_t dayRestrict = restrict->GetDayRestriction();
  for (size_t i = 0; i < 7; i++) {
    dayControls[i]->setChecked((dayRestrict & (1 << i)) != 0);
  }
}

std::shared_ptr<objects::SpawnRestriction> SpawnRestriction::Save() const {
  auto restrict = std::make_shared<objects::SpawnRestriction>();

  // Gather the button flag controls to set further down
  std::vector<QPushButton*> moonControls;
  for (int i = 0; i < prop->layoutMoonWax->count(); i++) {
    moonControls.push_back(
        (QPushButton*)prop->layoutMoonWax->itemAt(i)->widget());
  }

  for (int i = 0; i < prop->layoutMoonWane->count(); i++) {
    moonControls.push_back(
        (QPushButton*)prop->layoutMoonWane->itemAt(i)->widget());
  }

  std::vector<QPushButton*> dayControls;
  for (int i = 0; i < prop->layoutDay->count(); i++) {
    dayControls.push_back((QPushButton*)prop->layoutDay->itemAt(i)->widget());
  }

  restrict->SetDisabled(prop->disabled->isChecked());

  restrict->ClearTimeRestriction();
  for (auto& pair : prop->time->SaveUnsigned()) {
    restrict->SetTimeRestriction((uint16_t)pair.first, (uint16_t)pair.second);
  }

  restrict->ClearSystemTimeRestriction();
  for (auto& pair : prop->systemTime->SaveUnsigned()) {
    restrict->SetSystemTimeRestriction((uint16_t)pair.first,
                                       (uint16_t)pair.second);
  }

  restrict->ClearDateRestriction();
  for (auto& pair : prop->date->SaveUnsigned()) {
    restrict->SetDateRestriction((uint16_t)pair.first, (uint16_t)pair.second);
  }

  uint16_t moonRestrict = 0;
  for (size_t i = 0; i < 16; i++) {
    if (moonControls[i]->isChecked()) {
      moonRestrict = (uint16_t)(moonRestrict | (1 << i));
    }
  }
  restrict->SetMoonRestriction(moonRestrict);

  uint8_t dayRestrict = 0;
  for (size_t i = 0; i < 7; i++) {
    if (dayControls[i]->isChecked()) {
      dayRestrict = (uint8_t)(dayRestrict | (1 << i));
    }
  }
  restrict->SetDayRestriction(dayRestrict);

  return restrict;
}
