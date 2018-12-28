/**
 * @file tools/cathedral/src/EventWindow.h
 * @ingroup cathedral
 *
 * @author HACKfrost
 *
 * @brief Definition for a handler for XML utility operations.
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

#ifndef TOOLS_CATHEDRAL_SRC_XMLHANDLER_H
#define TOOLS_CATHEDRAL_SRC_XMLHANDLER_H

// libcomp Includes
#include <CString.h>
#include <Object.h>

class XmlTemplateObject;

class XmlHandler
{
public:
    XmlHandler() {}

    static void SimplifyObject(std::list<tinyxml2::XMLNode*> nodes);

    static std::shared_ptr<XmlTemplateObject> GetTemplateObject(
        const libcomp::String& objType, tinyxml2::XMLDocument& templateDoc);
};

#endif // TOOLS_CATHEDRAL_SRC_XMLHANDLER_H
