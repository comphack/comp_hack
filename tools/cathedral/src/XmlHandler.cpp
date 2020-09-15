/**
 * @file tools/cathedral/src/XmlHandler.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for a handler for XML utility operations.
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

#include "XmlHandler.h"

// object Includes
#include <Action.h>
#include <DropSet.h>
#include <Event.h>
#include <EventBase.h>
#include <EventChoice.h>
#include <EventCondition.h>
#include <ItemDrop.h>
#include <PlasmaSpawn.h>
#include <ServerBazaar.h>
#include <ServerNPC.h>
#include <ServerObject.h>
#include <ServerZone.h>
#include <ServerZonePartial.h>
#include <ServerZoneSpot.h>
#include <ServerZoneTrigger.h>
#include <Spawn.h>
#include <SpawnGroup.h>
#include <SpawnLocationGroup.h>
#include <SpawnRestriction.h>

// C++11 Standard Includes
#include <map>

class XmlTemplateObject {
 public:
  std::shared_ptr<libcomp::Object> Template;
  std::unordered_map<libcomp::String, tinyxml2::XMLNode*> MemberNodes;
  std::set<libcomp::String> CorrectMaps;
  std::set<libcomp::String> KeepDefaults;
  std::set<libcomp::String> ToHex;
  libcomp::String LastLesserMember;
  bool DropObjectIdentifier = false;
};

void XmlHandler::SimplifyObjects(std::list<tinyxml2::XMLNode*> nodes) {
  // Collect all object nodes and simplify by removing defaulted fields.
  // Also remove CDATA blocks as events are not complicated enough to
  // benefit from it.
  std::set<tinyxml2::XMLNode*> currentNodes;
  std::set<tinyxml2::XMLNode*> objectNodes;
  for (auto node : nodes) {
    currentNodes.insert(node);
  }

  while (currentNodes.size() > 0) {
    auto node = *currentNodes.begin();
    currentNodes.erase(node);

    auto elem = node->ToElement();
    if (elem && libcomp::String(elem->Name()) == "object") {
      objectNodes.insert(node);
    }

    auto child = node->FirstChild();
    while (child) {
      auto txt = child->ToText();
      if (txt) {
        txt->SetCData(false);
      } else {
        currentNodes.insert(child);
      }

      child = child->NextSibling();
    }
  }

  if (objectNodes.size() == 0) {
    return;
  }

  // Create an empty template object for each type encountered for comparison
  tinyxml2::XMLDocument templateDoc;
  std::unordered_map<libcomp::String, std::shared_ptr<XmlTemplateObject>>
      templateObjects;

  auto rootElem = templateDoc.NewElement("objects");
  templateDoc.InsertEndChild(rootElem);

  for (auto objNode : objectNodes) {
    libcomp::String objType(objNode->ToElement()->Attribute("name"));

    auto templateIter = templateObjects.find(objType);
    if (templateIter == templateObjects.end()) {
      auto tObj = GetTemplateObject(objType, templateDoc);
      if (tObj) {
        templateObjects[objType] = tObj;
        templateIter = templateObjects.find(objType);
      } else {
        // No simplification
        continue;
      }
    }

    auto tObj = templateIter->second;
    if (!tObj->LastLesserMember.IsEmpty()) {
      // Move the ID to the top (if it exists) and move less important
      // base properties to the bottom
      std::set<libcomp::String> seen;

      tinyxml2::XMLNode* lastComment = 0;

      auto child = objNode->FirstChild();
      while (child) {
        auto next = child->NextSibling();
        auto elem = child->ToElement();

        if (!elem) {
          if (dynamic_cast<const tinyxml2::XMLComment*>(child)) {
            lastComment = child;
          }

          child = next;
          continue;
        }

        libcomp::String member(elem->Attribute("name"));
        bool last = !next || seen.find(member) != seen.end();

        seen.insert(member);

        if (member == "ID") {
          // Move to the top (after comments)
          if (lastComment) {
            objNode->InsertAfterChild(lastComment, child);
          } else {
            objNode->InsertFirstChild(child);
          }
        } else if (!last && member != "next" && member != "queueNext") {
          // Move all others to the bottom
          objNode->InsertEndChild(child);
        }

        if (last || member == tObj->LastLesserMember) {
          break;
        } else {
          child = next;
        }
      }
    }

    if (tObj->DropObjectIdentifier) {
      objNode->ToElement()->DeleteAttribute("name");
    }

    // Drop matching level 1 child nodes (anything further down should not
    // be simplified anyway)
    auto child = objNode->FirstChild();
    while (child) {
      auto next = child->NextSibling();

      auto elem = child->ToElement();
      if (elem && libcomp::String(elem->Name()) == "member") {
        libcomp::String member(elem->Attribute("name"));

        auto iter = tObj->MemberNodes.find(member);
        if (iter != tObj->MemberNodes.end() && member != "ID") {
          if (tObj->CorrectMaps.find(member) != tObj->CorrectMaps.end()) {
            CorrectMap(child);
          }

          if (tObj->KeepDefaults.find(member) == tObj->KeepDefaults.end()) {
            auto child2 = iter->second;

            auto gc = child->FirstChild();
            auto gc2 = child2->FirstChild();

            auto txt = gc ? gc->ToText() : 0;
            auto txt2 = gc2 ? gc2->ToText() : 0;

            // If both have no child or both have the same text
            // representation, the nodes match
            if ((gc == 0 && gc2 == 0) || (txt && txt2 &&
                                          libcomp::String(txt->Value()) ==
                                              libcomp::String(txt2->Value()))) {
              // Default value matches, drop node
              objNode->DeleteChild(child);
            }
          }
        }
      }

      child = next;
    }
  }
}

void XmlHandler::CorrectMap(tinyxml2::XMLNode* parentNode) {
  std::map<uint32_t, tinyxml2::XMLNode*> mapped;

  auto pair = parentNode->FirstChild();
  while (pair != 0) {
    auto key = pair->FirstChildElement("key");
    if (key) {
      uint32_t k = libcomp::String(key->FirstChild()->ToText()->Value())
                       .ToInteger<uint32_t>();
      mapped[k] = pair;
    }

    pair = pair->NextSibling();
  }

  for (auto& mPair : mapped) {
    parentNode->InsertEndChild(mPair.second);
  }
}

std::list<libcomp::String> XmlHandler::GetComments(tinyxml2::XMLNode* node) {
  std::list<libcomp::String> comments;
  if (node) {
    auto child = node->FirstChild();
    while (child != 0) {
      auto comment = dynamic_cast<const tinyxml2::XMLComment*>(child);
      if (comment) {
        comments.push_back(libcomp::String(comment->Value()).Trimmed());
      } else {
        for (auto str : GetComments(child)) {
          comments.push_back(str);
        }
      }

      child = child->NextSibling();
    }
  }

  return comments;
}

std::shared_ptr<XmlTemplateObject> XmlHandler::GetTemplateObject(
    const libcomp::String& objType, tinyxml2::XMLDocument& templateDoc) {
  std::shared_ptr<libcomp::Object> obj;
  std::set<libcomp::String> correctMaps;
  std::set<libcomp::String> keepDefaults;
  libcomp::String lesserMember;
  bool dropObjectIdentifier = false;

  if (objType == "EventBase") {
    obj = std::make_shared<objects::EventBase>();
    dropObjectIdentifier = true;
  } else if (objType == "EventChoice") {
    obj = std::make_shared<objects::EventChoice>();
    lesserMember = "branchScriptParams";
  } else if (objType.Left(6) == "Action") {
    // Action derived
    obj = objects::Action::InheritedConstruction(objType);
    lesserMember = "transformScriptParams";
  } else if (objType.Left(5) == "Event") {
    if (objType.Right(9) == "Condition") {
      // EventCondition derived
      obj = objects::EventCondition::InheritedConstruction(objType);
    } else {
      // Event derived
      obj = objects::Event::InheritedConstruction(objType);
      lesserMember = "transformScriptParams";
    }
  } else if (objType == "DropSet") {
    obj = std::make_shared<objects::DropSet>();
  } else if (objType == "ItemDrop") {
    obj = std::make_shared<objects::ItemDrop>();
    dropObjectIdentifier = true;

    keepDefaults.insert("MaxStack");
  } else if (objType == "PlasmaSpawn") {
    obj = std::make_shared<objects::PlasmaSpawn>();
    lesserMember = "FailActions";
  } else if (objType == "ServerBazaar") {
    obj = std::make_shared<objects::ServerBazaar>();
    lesserMember = "MarketIDs";
  } else if (objType == "ServerNPC") {
    obj = std::make_shared<objects::ServerNPC>();
    lesserMember = "Actions";
  } else if (objType == "ServerObject") {
    obj = std::make_shared<objects::ServerObject>();
    lesserMember = "Actions";
  } else if (objType == "ServerObjectBase") {
    obj = std::make_shared<objects::ServerObjectBase>();
    lesserMember = "Rotation";
  } else if (objType == "ServerZone") {
    obj = std::make_shared<objects::ServerZone>();

    // Keep some defaults
    keepDefaults.insert("Global");
    keepDefaults.insert("StartingX");
    keepDefaults.insert("StartingY");
    keepDefaults.insert("StartingRotation");
    keepDefaults.insert("NPCs");
    keepDefaults.insert("Objects");
    keepDefaults.insert("Spots");

    correctMaps.insert("NPCs");
    correctMaps.insert("Objects");
    correctMaps.insert("Spawns");
    correctMaps.insert("SpawnGroups");
    correctMaps.insert("SpawnLocationGroups");
    correctMaps.insert("Spots");
  } else if (objType == "ServerZonePartial") {
    obj = std::make_shared<objects::ServerZonePartial>();

    correctMaps.insert("NPCs");
    correctMaps.insert("Objects");
    correctMaps.insert("Spawns");
    correctMaps.insert("SpawnGroups");
    correctMaps.insert("SpawnLocationGroups");
    correctMaps.insert("Spots");
  } else if (objType == "Spawn") {
    obj = std::make_shared<objects::Spawn>();
  } else if (objType == "SpawnGroup") {
    obj = std::make_shared<objects::SpawnGroup>();
  } else if (objType == "SpawnLocationGroup") {
    obj = std::make_shared<objects::SpawnLocationGroup>();
  } else if (objType == "SpawnRestriction") {
    obj = std::make_shared<objects::SpawnRestriction>();
  } else if (objType == "ServerZoneSpot") {
    obj = std::make_shared<objects::ServerZoneSpot>();
  } else if (objType == "ServerZoneTrigger") {
    obj = std::make_shared<objects::ServerZoneTrigger>();
  }

  if (obj) {
    auto rootElem = templateDoc.FirstChild()->ToElement();
    obj->Save(templateDoc, *rootElem);

    auto tNode = rootElem->LastChild();

    std::unordered_map<libcomp::String, tinyxml2::XMLNode*> tMembers;
    auto child = tNode->FirstChild();
    while (child) {
      auto elem = child->ToElement();
      if (elem && libcomp::String(elem->Name()) == "member") {
        // Remove CDATA here too
        auto gChild = child->FirstChild();
        auto txt = gChild ? gChild->ToText() : 0;
        if (txt && txt->CData()) {
          txt->SetCData(false);
        }

        libcomp::String memberName(elem->Attribute("name"));
        tMembers[memberName] = child;
      }

      child = child->NextSibling();
    }

    auto tObj = std::make_shared<XmlTemplateObject>();
    tObj->Template = obj;
    tObj->CorrectMaps = correctMaps;
    tObj->KeepDefaults = keepDefaults;
    tObj->LastLesserMember = lesserMember;
    tObj->MemberNodes = tMembers;
    tObj->DropObjectIdentifier = dropObjectIdentifier;

    return tObj;
  }

  return nullptr;
}
