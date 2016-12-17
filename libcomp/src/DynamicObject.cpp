/**
 * @file libcomp/src/DynamicObject.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Dynamically generated object.
 *
 * This file is part of the COMP_hack Library (libcomp).
 *
 * Copyright (C) 2012-2016 COMP_hack Team <compomega@tutanota.com>
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

#include "DynamicObject.h"

// libcomp Includes
#include "DynamicVariableFactory.h"

using namespace libcomp;

DynamicObject::DynamicObject(const std::shared_ptr<
    libobjgen::MetaObject>& metaObject) : mMetaData(metaObject)
{
    DynamicVariableFactory factory;

    for(auto it = metaObject->VariablesBegin();
        it != metaObject->VariablesEnd(); ++it)
    {
        auto var = *it;
        auto dynamicVar = factory.Create(var);

        mVariables.push_back(dynamicVar);
        mVariableLookup[var->GetName()] = dynamicVar;
    }
}

bool DynamicObject::IsValid(bool recursive) const
{
    (void)recursive;

    /// @todo Fix.
    return false;
}

/*bool DynamicObject::Load(ObjectInStream& stream)
{
    for(auto var : mVariables)
    {
        if(!var->Load(stream))
        {
            return false;
        }
    }

    return true;
}

bool DynamicObject::Save(ObjectOutStream& stream) const
{
    for(auto var : mVariables)
    {
        if(!var->Save(stream))
        {
            return false;
        }
    }

    return true;
}*/

uint16_t DynamicObject::GetDynamicSizeCount() const
{
    return mMetaData->GetDynamicSizeCount();
}
