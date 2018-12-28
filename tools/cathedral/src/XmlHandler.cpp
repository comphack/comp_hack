/**
 * @file tools/cathedral/src/XmlHandler.cpp
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Implementation for a handler for XML utility operations.
 *
 * Copyright (C) 2012-2018 COMP_hack Team <compomega@tutanota.com>
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
#include <Event.h>
#include <EventBase.h>
#include <EventChoice.h>
#include <EventCondition.h>

class XmlTemplateObject
{
public:
    std::shared_ptr<libcomp::Object> Template;
    std::unordered_map<libcomp::String, tinyxml2::XMLNode*> MemberNodes;
    libcomp::String LastLesserMember;
};

void XmlHandler::SimplifyObject(std::list<tinyxml2::XMLNode*> nodes)
{
    // Collect all object nodes and simplify by removing defaulted fields.
    // Also remove CDATA blocks as events are not complicated enough to
    // benefit from it.
    std::set<tinyxml2::XMLNode*> currentNodes;
    std::set<tinyxml2::XMLNode*> objectNodes;
    for(auto node : nodes)
    {
        currentNodes.insert(node);
    }

    while(currentNodes.size() > 0)
    {
        auto node = *currentNodes.begin();
        currentNodes.erase(node);

        if(libcomp::String(node->ToElement()->Name()) == "object")
        {
            objectNodes.insert(node);
        }

        auto child = node->FirstChild();
        while(child)
        {
            auto txt = child->ToText();
            if(txt)
            {
                txt->SetCData(false);
            }
            else
            {
                currentNodes.insert(child);
            }

            child = child->NextSibling();
        }
    }

    if(objectNodes.size() == 0)
    {
        return;
    }

    // Create an empty template object for each type encountered for comparison
    tinyxml2::XMLDocument templateDoc;
    std::unordered_map<libcomp::String,
        std::shared_ptr<XmlTemplateObject>> templateObjects;

    auto rootElem = templateDoc.NewElement("objects");
    templateDoc.InsertEndChild(rootElem);

    for(auto objNode : objectNodes)
    {
        libcomp::String objType(objNode->ToElement()->Attribute("name"));

        auto templateIter = templateObjects.find(objType);
        if(templateIter == templateObjects.end())
        {
            auto tObj = GetTemplateObject(objType, templateDoc);
            if(tObj)
            {
                templateObjects[objType] = tObj;
                templateIter = templateObjects.find(objType);
            }
            else
            {
                // No simplification
                continue;
            }
        }

        auto tObj = templateIter->second;
        if(!tObj->LastLesserMember.IsEmpty())
        {
            // Move the ID to the top (if it exists) and move less important
            // base properties to the bottom
            std::set<libcomp::String> seen;

            auto child = objNode->FirstChild();
            while(child)
            {
                auto next = child->NextSibling();

                libcomp::String memberName(child->ToElement()
                    ->Attribute("name"));
                bool last = memberName == tObj->LastLesserMember || !next ||
                    seen.find(memberName) != seen.end();

                seen.insert(memberName);

                if(memberName == "ID")
                {
                    // Move to the top
                    objNode->InsertFirstChild(child);
                }
                else if(!last &&
                    memberName != "next" && memberName != "queueNext")
                {
                    // Move all others to the bottom
                    objNode->InsertEndChild(child);
                }

                if(last)
                {
                    break;
                }
                else
                {
                    child = next;
                }
            }
        }

        if(objType == "EventBase")
        {
            // EventBase is used for the branch structure which does not need
            // the object identifier and often times these can be very simple
            // so drop it here.
            objNode->ToElement()->DeleteAttribute("name");
        }

        // Drop matching level 1 child nodes (anything further down should not
        // be simplified anyway)
        auto child = objNode->FirstChild();
        while(child)
        {
            auto next = child->NextSibling();

            auto elem = child->ToElement();
            if(elem)
            {
                libcomp::String memberName(elem->Attribute("name"));

                auto iter = tObj->MemberNodes.find(memberName);
                if(iter != tObj->MemberNodes.end())
                {
                    auto child2 = iter->second;

                    auto gc = child->FirstChild();
                    auto gc2 = child2->FirstChild();

                    auto txt = gc ? gc->ToText() : 0;
                    auto txt2 = gc2 ? gc2->ToText() : 0;

                    // If both have no child or both have the same text
                    // representation, the nodes match
                    if((gc == 0 && gc2 == 0) || (txt && txt2 &&
                        libcomp::String(txt->Value()) ==
                        libcomp::String(txt2->Value())))
                    {
                        // Default value matches, drop node
                        objNode->DeleteChild(child);
                    }
                }
            }

            child = next;
        }
    }
}

std::shared_ptr<XmlTemplateObject> XmlHandler::GetTemplateObject(
    const libcomp::String& objType, tinyxml2::XMLDocument& templateDoc)
{
    std::shared_ptr<libcomp::Object> obj;
    libcomp::String lesserMember;

    if(objType == "EventBase")
    {
        obj = std::make_shared<objects::EventBase>();
        lesserMember = "popNext";
    }
    else if(objType == "EventChoice")
    {
        obj = std::make_shared<objects::EventChoice>();
        lesserMember = "branchScriptParams";
    }
    else if(objType.Left(6) == "Action")
    {
        // Action derived
        obj = objects::Action::InheritedConstruction(objType);
        lesserMember = "transformScriptParams";
    }
    else if(objType.Left(5) == "Event")
    {
        if(objType.Right(9) == "Condition")
        {
            // EventCondition derived
            obj = objects::EventCondition::InheritedConstruction(
                objType);
        }
        else
        {
            // Event derived
            obj = objects::Event::InheritedConstruction(objType);
            lesserMember = "transformScriptParams";
        }
    }

    if(obj)
    {
        auto rootElem = templateDoc.FirstChild()->ToElement();
        obj->Save(templateDoc, *rootElem);

        auto tNode = rootElem->LastChild();

        std::unordered_map<libcomp::String,
            tinyxml2::XMLNode*> tMembers;
        auto child = tNode->FirstChild();
        while(child)
        {
            auto elem = child->ToElement();
            if(elem && libcomp::String(elem->Name()) == "member")
            {
                // Remove CDATA here too
                auto gChild = child->FirstChild();
                auto txt = gChild ? gChild->ToText() : 0;
                if(txt && txt->CData())
                {
                    txt->SetCData(false);
                }

                libcomp::String memberName(elem->Attribute("name"));
                tMembers[memberName] = child;
            }

            child = child->NextSibling();
        }

        auto tObj = std::make_shared<XmlTemplateObject>();
        tObj->Template = obj;
        tObj->LastLesserMember = lesserMember;
        tObj->MemberNodes = tMembers;

        return tObj;
    }

    return nullptr;
}
