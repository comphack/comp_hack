/**
 * @file tools/updater/src/ClientPatches.h
 * @ingroup tools
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Reads and writes the comp_client(-user).xml file.
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

#ifndef TOOLS_UPDATER_SRC_CLIENTPATCHES_H
#define TOOLS_UPDATER_SRC_CLIENTPATCHES_H

// Ignore warnings
#include <PushIgnore.h>

#include <QDomDocument>
#include <QStringList>

// Stop ignoring warnings
#include <PopIgnore.h>

class QCheckBox;

class ClientPatches {
 public:
  ClientPatches(ClientPatches* pBase = nullptr);

  bool Load(const QString& path);
  bool Save(const QString& path);
  void Clear();

  void ApplyEnforcement(const ClientPatches* pBase);
  void ApplyEnforcement(const QString& patchName, QCheckBox* pCheckBox);

  bool GetBlowfishKey() const;
  void SetBlowfishKey(bool enabled);

  bool GetNoWebAuth() const;
  void SetNoWebAuth(bool enabled);

  bool GetPackFile() const;
  void SetPackFile(bool enabled);

  bool GetChatTimestampFirst() const;
  void SetChatTimestampFirst(bool enabled);

  bool GetExtendedBuffTimerDisplay() const;
  void SetExtendedBuffTimerDisplay(bool enabled);

  bool GetExtendedEXPDisplay() const;
  void SetExtendedEXPDisplay(bool enabled);

  bool GetInfiniteZoom() const;
  void SetInfiniteZoom(bool enabled);

  bool GetCharacterNameCheck() const;
  void SetCharacterNameCheck(bool enabled);

  bool GetLobbyIME() const;
  void SetLobbyIME(bool enabled);

  bool GetServerPrime() const;
  void SetServerPrime(bool enabled);

  bool GetTranslation() const;
  void SetTranslation(bool enabled);

  bool GetChannelTransfer() const;
  void SetChannelTransfer(bool enabled);

  bool GetCustomPackets() const;
  void SetCustomPackets(bool enabled);

  bool GetUpdaterCheck() const;
  void SetUpdaterCheck(bool enabled);

  bool GetLocale() const;
  void SetLocale(bool enabled);

  bool GetSoundtrackPatch() const;
  void SetSoundtrackPatch(bool enabled);

  bool GetKillCounterSpacing() const;
  void SetKillCounterSpacing(bool enabled);

  bool GetAccountDump() const;
  void SetAccountDump(bool enabled);

  QString GetSoundtrack() const;
  void SetSoundtrack(const QString& soundtrack);

  bool GetAllowAll() const;
  QStringList GetRequiredPatches() const;
  QStringList GetAllowedPatches() const;
  QStringList GetBlockedPatches() const;

 private:
  void ApplyEnforcement(const ClientPatches* pBase, const QString& patchName,
                        bool& patchValue);

  bool LoadStringElement(const QDomElement& root, const QString& tag,
                         std::size_t baseValueOffset, QString& value,
                         QDomElement& element, const QString& attr = {},
                         const QString& attrValue = {});
  bool LoadStringElement(const QDomElement& root, const QString& tag,
                         const QString* pBaseValue, QString& value,
                         QDomElement& element, const QString& attr = {},
                         const QString& attrValue = {});
  bool LoadPatchElement(const QDomElement& root, const QString& patch,
                        std::size_t baseValueOffset, bool& value,
                        QDomElement& element);
  bool SaveStringElement(const QString& tag, std::size_t baseValueOffset,
                         const QString& value, QDomElement& element,
                         const QString& attr = {},
                         const QString& attrValue = {});
  bool SaveStringElement(const QString& tag, const QString* pBaseValue,
                         const QString& value, QDomElement& element,
                         const QString& attr = {},
                         const QString& attrValue = {});
  bool SavePatchElement(const QString& patch, std::size_t baseValueOffset,
                        bool value, QDomElement& element);
  QDomElement GetFirstElement(const QDomElement& root, const QString& tag,
                              const QString& attr = {},
                              const QString& attrValue = {});

  ClientPatches* mBase;
  QDomDocument mDoc;

  bool mBlowfishKey;
  QDomElement mBlowfishKeyElement;

  bool mNoWebAuth;
  QDomElement mNoWebAuthElement;

  bool mPackFile;
  QDomElement mPackFileElement;

  bool mChatTimestampFirst;
  QDomElement mChatTimestampFirstElement;

  bool mExtendedBuffTimerDisplay;
  QDomElement mExtendedBuffTimerDisplayElement;

  bool mExtendedEXPDisplay;
  QDomElement mExtendedEXPDisplayElement;

  bool mInfiniteZoom;
  QDomElement mInfiniteZoomElement;

  bool mCharacterNameCheck;
  QDomElement mCharacterNameCheckElement;

  bool mLobbyIME;
  QDomElement mLobbyIMEElement;

  bool mServerPrime;
  QDomElement mServerPrimeElement;

  bool mTranslation;
  QDomElement mTranslationElement;

  bool mChannelTransfer;
  QDomElement mChannelTransferElement;

  bool mCustomPackets;
  QDomElement mCustomPacketsElement;

  bool mUpdaterCheck;
  QDomElement mUpdaterCheckElement;

  bool mLocale;
  QDomElement mLocaleElement;

  bool mSoundtrackPatch;
  QDomElement mSoundtrackPatchElement;

  bool mKillCounterSpacing;
  QDomElement mKillCounterSpacingElement;

  bool mAccountDump;
  QDomElement mAccountDumpElement;

  QString mSoundtrack;
  QDomElement mSoundtrackElement;

  bool mAllowAll;
  QStringList mRequiredPatches;
  QStringList mAllowedPatches;
  QStringList mBlockedPatches;
};

#endif  // TOOLS_UPDATER_SRC_CLIENTPATCHES_H
