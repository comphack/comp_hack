/**
 * @file tools/updater/src/ClientPatches.cpp
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

#include "ClientPatches.h"

// Ignore warnings
#include <PushIgnore.h>

#include <QCheckBox>
#include <QFile>
#include <QMessageBox>

// Stop ignoring warnings
#include <PopIgnore.h>

ClientPatches::ClientPatches(ClientPatches* pBase)
    : mBase(pBase),
      mBlowfishKey(true),
      mNoWebAuth(false),
      mPackFile(true),
      mChatTimestampFirst(true),
      mExtendedBuffTimerDisplay(true),
      mExtendedEXPDisplay(true),
      mInfiniteZoom(true),
      mCharacterNameCheck(true),
      mLobbyIME(true),
      mServerPrime(true),
      mTranslation(true),
      mChannelTransfer(true),
      mCustomPackets(true),
      mUpdaterCheck(true),
      mLocale(true),
      mSoundtrackPatch(true),
      mKillCounterSpacing(true),
      mAccountDump(true),
      mAllowAll(true) {}

bool ClientPatches::Load(const QString& path) {
  Clear();

  bool noFile = false;

  QFile file(path);
  if (!file.open(QIODevice::ReadOnly)) {
    noFile = true;
  }

  if (!mDoc.setContent(&file)) {
    noFile = true;
  }

  if (noFile) {
    mDoc.setContent(
        QString("<?xml version='1.0' encoding='UTF-8'?>\n<config/>\n"));
  }

  QDomElement root = mDoc.documentElement();

  if (!LoadPatchElement(root, "blowfishKey",
                        offsetof(ClientPatches, mBlowfishKey), mBlowfishKey,
                        mBlowfishKeyElement)) {
    return false;
  }

  if (!LoadPatchElement(root, "noWebAuth", offsetof(ClientPatches, mNoWebAuth),
                        mNoWebAuth, mNoWebAuthElement)) {
    return false;
  }

  if (!LoadPatchElement(root, "packFile", offsetof(ClientPatches, mPackFile),
                        mPackFile, mPackFileElement)) {
    return false;
  }

  if (!LoadPatchElement(root, "chatTimestampFirst",
                        offsetof(ClientPatches, mChatTimestampFirst),
                        mChatTimestampFirst, mChatTimestampFirstElement)) {
    return false;
  }

  if (!LoadPatchElement(root, "extendedBuffTimerDisplay",
                        offsetof(ClientPatches, mExtendedBuffTimerDisplay),
                        mExtendedBuffTimerDisplay,
                        mExtendedBuffTimerDisplayElement)) {
    return false;
  }

  if (!LoadPatchElement(root, "extendedEXPDisplay",
                        offsetof(ClientPatches, mExtendedEXPDisplay),
                        mExtendedEXPDisplay, mExtendedEXPDisplayElement)) {
    return false;
  }

  if (!LoadPatchElement(root, "infiniteZoom",
                        offsetof(ClientPatches, mInfiniteZoom), mInfiniteZoom,
                        mInfiniteZoomElement)) {
    return false;
  }

  if (!LoadPatchElement(root, "characterNameCheck",
                        offsetof(ClientPatches, mCharacterNameCheck),
                        mCharacterNameCheck, mCharacterNameCheckElement)) {
    return false;
  }

  if (!LoadPatchElement(root, "lobbyIME", offsetof(ClientPatches, mLobbyIME),
                        mLobbyIME, mLobbyIMEElement)) {
    return false;
  }

  if (!LoadPatchElement(root, "serverPrime",
                        offsetof(ClientPatches, mServerPrime), mServerPrime,
                        mServerPrimeElement)) {
    return false;
  }

  if (!LoadPatchElement(root, "translation",
                        offsetof(ClientPatches, mTranslation), mTranslation,
                        mTranslationElement)) {
    return false;
  }

  if (!LoadPatchElement(root, "channelTransfer",
                        offsetof(ClientPatches, mChannelTransfer),
                        mChannelTransfer, mChannelTransferElement)) {
    return false;
  }

  if (!LoadPatchElement(root, "customPackets",
                        offsetof(ClientPatches, mCustomPackets), mCustomPackets,
                        mCustomPacketsElement)) {
    return false;
  }

  if (!LoadPatchElement(root, "updaterCheck",
                        offsetof(ClientPatches, mUpdaterCheck), mUpdaterCheck,
                        mUpdaterCheckElement)) {
    return false;
  }

  if (!LoadPatchElement(root, "locale", offsetof(ClientPatches, mLocale),
                        mLocale, mLocaleElement)) {
    return false;
  }

  if (!LoadPatchElement(root, "soundtrackPatch",
                        offsetof(ClientPatches, mSoundtrackPatch),
                        mSoundtrackPatch, mSoundtrackPatchElement)) {
    return false;
  }

  if (!LoadStringElement(root, "soundtrack",
                         offsetof(ClientPatches, mSoundtrack), mSoundtrack,
                         mSoundtrackElement)) {
    return false;
  }

  if (!LoadPatchElement(root, "killCounterSpacing",
                        offsetof(ClientPatches, mKillCounterSpacing),
                        mKillCounterSpacing, mKillCounterSpacingElement)) {
    return false;
  }

  if (!LoadPatchElement(root, "accountDump",
                        offsetof(ClientPatches, mAccountDump), mAccountDump,
                        mAccountDumpElement)) {
    return false;
  }

  auto enforcement = GetFirstElement(root, "enforcement");

  if (!enforcement.isNull()) {
    auto allowAll = enforcement.attribute("allow-all", "true").toLower();
    mAllowAll = "true" == allowAll || "on" == allowAll || "yes" == allowAll;

    auto elements = root.elementsByTagName("require");

    for (int i = 0; i < elements.size(); ++i) {
      mRequiredPatches.append(elements.at(i).toElement().text().trimmed());
    }

    elements = root.elementsByTagName("allow");

    for (int i = 0; i < elements.size(); ++i) {
      mAllowedPatches.append(elements.at(i).toElement().text().trimmed());
    }

    elements = root.elementsByTagName("block");

    for (int i = 0; i < elements.size(); ++i) {
      mBlockedPatches.append(elements.at(i).toElement().text().trimmed());
    }
  }

  return true;
}

bool ClientPatches::Save(const QString& path) {
  if (!SavePatchElement("blowfishKey", offsetof(ClientPatches, mBlowfishKey),
                        mBlowfishKey, mBlowfishKeyElement)) {
    return false;
  }

  if (!SavePatchElement("noWebAuth", offsetof(ClientPatches, mNoWebAuth),
                        mNoWebAuth, mNoWebAuthElement)) {
    return false;
  }

  if (!SavePatchElement("packFile", offsetof(ClientPatches, mPackFile),
                        mPackFile, mPackFileElement)) {
    return false;
  }

  if (!SavePatchElement("chatTimestampFirst",
                        offsetof(ClientPatches, mChatTimestampFirst),
                        mChatTimestampFirst, mChatTimestampFirstElement)) {
    return false;
  }

  if (!SavePatchElement("extendedBuffTimerDisplay",
                        offsetof(ClientPatches, mExtendedBuffTimerDisplay),
                        mExtendedBuffTimerDisplay,
                        mExtendedBuffTimerDisplayElement)) {
    return false;
  }

  if (!SavePatchElement("extendedEXPDisplay",
                        offsetof(ClientPatches, mExtendedEXPDisplay),
                        mExtendedEXPDisplay, mExtendedEXPDisplayElement)) {
    return false;
  }

  if (!SavePatchElement("infiniteZoom", offsetof(ClientPatches, mInfiniteZoom),
                        mInfiniteZoom, mInfiniteZoomElement)) {
    return false;
  }

  if (!SavePatchElement("characterNameCheck",
                        offsetof(ClientPatches, mCharacterNameCheck),
                        mCharacterNameCheck, mCharacterNameCheckElement)) {
    return false;
  }

  if (!SavePatchElement("lobbyIME", offsetof(ClientPatches, mLobbyIME),
                        mLobbyIME, mLobbyIMEElement)) {
    return false;
  }

  if (!SavePatchElement("serverPrime", offsetof(ClientPatches, mServerPrime),
                        mServerPrime, mServerPrimeElement)) {
    return false;
  }

  if (!SavePatchElement("translation", offsetof(ClientPatches, mTranslation),
                        mTranslation, mTranslationElement)) {
    return false;
  }

  if (!SavePatchElement("channelTransfer",
                        offsetof(ClientPatches, mChannelTransfer),
                        mChannelTransfer, mChannelTransferElement)) {
    return false;
  }

  if (!SavePatchElement("customPackets",
                        offsetof(ClientPatches, mCustomPackets), mCustomPackets,
                        mCustomPacketsElement)) {
    return false;
  }

  if (!SavePatchElement("updaterCheck", offsetof(ClientPatches, mUpdaterCheck),
                        mUpdaterCheck, mUpdaterCheckElement)) {
    return false;
  }

  if (!SavePatchElement("locale", offsetof(ClientPatches, mLocale), mLocale,
                        mLocaleElement)) {
    return false;
  }

  if (!SavePatchElement("soundtrackPatch",
                        offsetof(ClientPatches, mSoundtrackPatch),
                        mSoundtrackPatch, mSoundtrackPatchElement)) {
    return false;
  }

  if (!SaveStringElement("soundtrack", offsetof(ClientPatches, mSoundtrack),
                         mSoundtrack, mSoundtrackElement)) {
    return false;
  }

  if (!SavePatchElement("killCounterSpacing",
                        offsetof(ClientPatches, mKillCounterSpacing),
                        mKillCounterSpacing, mKillCounterSpacingElement)) {
    return false;
  }

  if (!SavePatchElement("accountDump", offsetof(ClientPatches, mAccountDump),
                        mAccountDump, mAccountDumpElement)) {
    return false;
  }

  QFile file(path);
  if (!file.open(QIODevice::WriteOnly)) {
    return false;
  }

  if (!file.write(mDoc.toString(4).toUtf8())) {
    return false;
  }

  return true;
}

void ClientPatches::Clear() {
  mBlowfishKeyElement = QDomElement();
  mBlowfishKey = true;

  mNoWebAuthElement = QDomElement();
  mNoWebAuth = false;

  mPackFileElement = QDomElement();
  mPackFile = false;

  mChatTimestampFirstElement = QDomElement();
  mChatTimestampFirst = true;

  mExtendedBuffTimerDisplayElement = QDomElement();
  mExtendedBuffTimerDisplay = true;

  mExtendedEXPDisplayElement = QDomElement();
  mExtendedEXPDisplay = true;

  mInfiniteZoomElement = QDomElement();
  mInfiniteZoom = true;

  mCharacterNameCheckElement = QDomElement();
  mCharacterNameCheck = true;

  mLobbyIMEElement = QDomElement();
  mLobbyIME = true;

  mServerPrimeElement = QDomElement();
  mServerPrime = true;

  mTranslationElement = QDomElement();
  mTranslation = true;

  mChannelTransferElement = QDomElement();
  mChannelTransfer = true;

  mCustomPacketsElement = QDomElement();
  mCustomPackets = true;

  mUpdaterCheckElement = QDomElement();
  mUpdaterCheck = true;

  mLocaleElement = QDomElement();
  mLocale = true;

  mSoundtrackPatchElement = QDomElement();
  mSoundtrackPatch = true;

  mKillCounterSpacingElement = QDomElement();
  mKillCounterSpacing = true;

  mAccountDumpElement = QDomElement();
  mAccountDump = true;

  mSoundtrackElement = QDomElement();
  mSoundtrack.clear();

  mDoc = QDomDocument();
  mDoc.appendChild(mDoc.createProcessingInstruction(
      "xml", "version=\"1.0\" encoding=\"UTF-8\""));
  mDoc.appendChild(mDoc.createElement("config"));

  mAllowAll = true;
  mRequiredPatches.clear();
  mAllowedPatches.clear();
  mBlockedPatches.clear();
}

void ClientPatches::ApplyEnforcement(const ClientPatches* pBase) {
  ApplyEnforcement(pBase, "blowfishKey", mBlowfishKey);
  ApplyEnforcement(pBase, "noWebAuth", mNoWebAuth);
  ApplyEnforcement(pBase, "packFile", mPackFile);
  ApplyEnforcement(pBase, "chatTimestampFirst", mChatTimestampFirst);
  ApplyEnforcement(pBase, "extendedBuffTimerDisplay",
                   mExtendedBuffTimerDisplay);
  ApplyEnforcement(pBase, "extendedEXPDisplay", mExtendedEXPDisplay);
  ApplyEnforcement(pBase, "infiniteZoom", mInfiniteZoom);
  ApplyEnforcement(pBase, "characterNameCheck", mCharacterNameCheck);
  ApplyEnforcement(pBase, "lobbyIME", mLobbyIME);
  ApplyEnforcement(pBase, "serverPrime", mServerPrime);
  ApplyEnforcement(pBase, "translation", mTranslation);
  ApplyEnforcement(pBase, "channelTransfer", mChannelTransfer);
  ApplyEnforcement(pBase, "customPackets", mCustomPackets);
  ApplyEnforcement(pBase, "updaterCheck", mUpdaterCheck);
  ApplyEnforcement(pBase, "locale", mLocale);
  ApplyEnforcement(pBase, "soundtrackPatch", mSoundtrackPatch);
  ApplyEnforcement(pBase, "killCounterSpacing", mKillCounterSpacing);
  ApplyEnforcement(pBase, "accountDump", mAccountDump);
}

void ClientPatches::ApplyEnforcement(const QString& patchName,
                                     QCheckBox* pCheckBox) {
  if (mAllowAll) {
    if (mRequiredPatches.contains(patchName) ||
        mBlockedPatches.contains(patchName)) {
      pCheckBox->setEnabled(false);
    }
  } else {
    if (!mAllowedPatches.contains(patchName)) {
      pCheckBox->setEnabled(false);
    }
  }
}

void ClientPatches::ApplyEnforcement(const ClientPatches* pBase,
                                     const QString& patchName,
                                     bool& patchValue) {
  if (mAllowAll) {
    if (pBase->mRequiredPatches.contains(patchName)) {
      patchValue = true;
    } else if (pBase->mBlockedPatches.contains(patchName)) {
      patchValue = false;
    }
  } else {
    if (!pBase->mRequiredPatches.contains(patchName) &&
        !pBase->mAllowedPatches.contains(patchName)) {
      patchValue = false;
    }
  }
}

bool ClientPatches::GetBlowfishKey() const { return mBlowfishKey; }

void ClientPatches::SetBlowfishKey(bool enabled) { mBlowfishKey = enabled; }

bool ClientPatches::GetNoWebAuth() const { return mNoWebAuth; }

void ClientPatches::SetNoWebAuth(bool enabled) { mNoWebAuth = enabled; }

bool ClientPatches::GetPackFile() const { return mPackFile; }

void ClientPatches::SetPackFile(bool enabled) { mPackFile = enabled; }

bool ClientPatches::GetChatTimestampFirst() const {
  return mChatTimestampFirst;
}

void ClientPatches::SetChatTimestampFirst(bool enabled) {
  mChatTimestampFirst = enabled;
}

bool ClientPatches::GetExtendedBuffTimerDisplay() const {
  return mExtendedBuffTimerDisplay;
}

void ClientPatches::SetExtendedBuffTimerDisplay(bool enabled) {
  mExtendedBuffTimerDisplay = enabled;
}

bool ClientPatches::GetExtendedEXPDisplay() const {
  return mExtendedEXPDisplay;
}

void ClientPatches::SetExtendedEXPDisplay(bool enabled) {
  mExtendedEXPDisplay = enabled;
}

bool ClientPatches::GetInfiniteZoom() const { return mInfiniteZoom; }

void ClientPatches::SetInfiniteZoom(bool enabled) { mInfiniteZoom = enabled; }

bool ClientPatches::GetCharacterNameCheck() const {
  return mCharacterNameCheck;
}

void ClientPatches::SetCharacterNameCheck(bool enabled) {
  mCharacterNameCheck = enabled;
}

bool ClientPatches::GetLobbyIME() const { return mLobbyIME; }

void ClientPatches::SetLobbyIME(bool enabled) { mLobbyIME = enabled; }

bool ClientPatches::GetServerPrime() const { return mServerPrime; }

void ClientPatches::SetServerPrime(bool enabled) { mServerPrime = enabled; }

bool ClientPatches::GetTranslation() const { return mTranslation; }

void ClientPatches::SetTranslation(bool enabled) { mTranslation = enabled; }

bool ClientPatches::GetChannelTransfer() const { return mChannelTransfer; }

void ClientPatches::SetChannelTransfer(bool enabled) {
  mChannelTransfer = enabled;
}

bool ClientPatches::GetCustomPackets() const { return mCustomPackets; }

void ClientPatches::SetCustomPackets(bool enabled) { mCustomPackets = enabled; }

bool ClientPatches::GetUpdaterCheck() const { return mUpdaterCheck; }

void ClientPatches::SetUpdaterCheck(bool enabled) { mUpdaterCheck = enabled; }

bool ClientPatches::GetLocale() const { return mLocale; }

void ClientPatches::SetLocale(bool enabled) { mLocale = enabled; }

bool ClientPatches::GetSoundtrackPatch() const { return mSoundtrackPatch; }

void ClientPatches::SetSoundtrackPatch(bool enabled) {
  mSoundtrackPatch = enabled;
}

bool ClientPatches::GetKillCounterSpacing() const {
  return mKillCounterSpacing;
}

void ClientPatches::SetKillCounterSpacing(bool enabled) {
  mKillCounterSpacing = enabled;
}

bool ClientPatches::GetAccountDump() const { return mAccountDump; }

void ClientPatches::SetAccountDump(bool enabled) { mAccountDump = enabled; }

QString ClientPatches::GetSoundtrack() const { return mSoundtrack; }

void ClientPatches::SetSoundtrack(const QString& soundtrack) {
  mSoundtrack = soundtrack;
}

bool ClientPatches::GetAllowAll() const { return mAllowAll; }

QStringList ClientPatches::GetRequiredPatches() const {
  return mRequiredPatches;
}

QStringList ClientPatches::GetAllowedPatches() const { return mAllowedPatches; }

QStringList ClientPatches::GetBlockedPatches() const { return mBlockedPatches; }

bool ClientPatches::LoadStringElement(const QDomElement& root,
                                      const QString& tag,
                                      std::size_t baseValueOffset,
                                      QString& value, QDomElement& element,
                                      const QString& attr,
                                      const QString& attrValue) {
  QString* pBaseValue =
      mBase ? (QString*)((char*)mBase + baseValueOffset) : nullptr;

  return LoadStringElement(root, tag, pBaseValue, value, element, attr,
                           attrValue);
}

bool ClientPatches::LoadStringElement(const QDomElement& root,
                                      const QString& tag,
                                      const QString* pBaseValue, QString& value,
                                      QDomElement& element, const QString& attr,
                                      const QString& attrValue) {
  element = GetFirstElement(root, tag, attr, attrValue);

  if (!element.isNull()) {
    value = element.toElement().text().trimmed();
  } else if (pBaseValue) {
    value = *pBaseValue;
  } else {
    value.clear();
  }

  return true;
}

bool ClientPatches::LoadPatchElement(const QDomElement& root,
                                     const QString& patch,
                                     std::size_t baseValueOffset, bool& value,
                                     QDomElement& element) {
  bool* pBaseValue = nullptr;
  QString baseValue;
  QString stringValue;

  if (mBase) {
    pBaseValue = (bool*)((char*)mBase + baseValueOffset);
    baseValue = *pBaseValue ? "apply" : "skip";
  }

  if (!LoadStringElement(root, "patch", mBase ? &baseValue : nullptr,
                         stringValue, element, "name", patch)) {
    return false;
  }

  value = "apply" == stringValue.toLower();

  return true;
}

bool ClientPatches::SaveStringElement(const QString& tag,
                                      std::size_t baseValueOffset,
                                      const QString& value,
                                      QDomElement& element, const QString& attr,
                                      const QString& attrValue) {
  QString* pBaseValue =
      mBase ? (QString*)((char*)mBase + baseValueOffset) : nullptr;

  return SaveStringElement(tag, pBaseValue, value, element, attr, attrValue);
}

bool ClientPatches::SaveStringElement(const QString& tag,
                                      const QString* pBaseValue,
                                      const QString& value,
                                      QDomElement& element, const QString& attr,
                                      const QString& attrValue) {
  if (pBaseValue && *pBaseValue == value) {
    // We match the base settings, remove the custom option.
    if (!element.isNull()) {
      mDoc.documentElement().removeChild(element);
      element = QDomElement();
    }
  } else {
    // The custom option must be there.
    if (element.isNull()) {
      element = mDoc.createElement(tag);
      if (!attr.isEmpty()) {
        element.setAttribute(attr, attrValue);
      }
      element.appendChild(mDoc.createTextNode(value));
      mDoc.documentElement().appendChild(element);
    } else {
      if (element.hasChildNodes()) {
        element.firstChild().setNodeValue(value);
      } else {
        element.appendChild(mDoc.createTextNode(value));
      }
    }
  }

  return true;
}

bool ClientPatches::SavePatchElement(const QString& patch,
                                     std::size_t baseValueOffset, bool value,
                                     QDomElement& element) {
  bool* pBaseValue = nullptr;
  QString baseValue;
  QString stringValue = value ? "apply" : "skip";

  if (mBase) {
    pBaseValue = (bool*)((char*)mBase + baseValueOffset);
    baseValue = *pBaseValue ? "apply" : "skip";
  }

  if (!SaveStringElement("patch", mBase ? &baseValue : nullptr, stringValue,
                         element, "name", patch)) {
    return false;
  }

  return true;
}

QDomElement ClientPatches::GetFirstElement(const QDomElement& root,
                                           const QString& tag,
                                           const QString& attr,
                                           const QString& attrValue) {
  QDomNode node;
  auto elements = root.elementsByTagName(tag);

  for (int i = 0; i < elements.size(); ++i) {
    if (attr.isEmpty()) {
      node = elements.at(i);
      break;
    } else if (elements.at(i).toElement().attribute(attr) == attrValue) {
      node = elements.at(i);
      break;
    }
  }

  return node.toElement();
}
