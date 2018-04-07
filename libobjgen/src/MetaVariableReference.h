/**
 * @file libobjgen/src/MetaVariableReference.h
 * @ingroup libobjgen
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Meta data for a member variable that is a reference to another object.
 *
 * This file is part of the COMP_hack Object Generator Library (libobjgen).
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

#ifndef LIBOBJGEN_SRC_METAVARIABLEREFERENCE_H
#define LIBOBJGEN_SRC_METAVARIABLEREFERENCE_H

// libobjgen Includes
#include "MetaVariable.h"

// Standard C++11 Includes
#include <list>

namespace libobjgen
{

class MetaVariableReference : public MetaVariable
{
public:
    MetaVariableReference();
    virtual ~MetaVariableReference();

    virtual size_t GetSize() const;

    virtual MetaVariableType_t GetMetaType() const;

    virtual std::string GetType() const;

    std::string GetReferenceType(bool includeNamespace = false) const;
    bool SetReferenceType(const std::string& referenceType);

    std::string GetNamespace() const;
    bool SetNamespace(const std::string& ns);

    bool IsPersistentReference() const;
    bool SetPersistentReference(bool persistentReference);

    bool IsScriptReference() const;
    bool SetScriptReference(bool scriptReference);

    bool IsGeneric() const;
    void SetGeneric();

    bool IsIndirect() const;

    bool GetNullDefault() const;
    bool SetNullDefault(bool nullDefault);

    void AddDefaultedVariable(std::shared_ptr<MetaVariable>& var);
    const std::list<std::shared_ptr<MetaVariable>> GetDefaultedVariables() const;

    virtual bool IsCoreType() const;
    virtual bool IsScriptAccessible() const;
    virtual bool IsValid() const;

    virtual bool Load(std::istream& stream);
    virtual bool Save(std::ostream& stream) const;

    virtual bool Load(const tinyxml2::XMLDocument& doc,
        const tinyxml2::XMLElement& root);
    virtual bool Save(tinyxml2::XMLDocument& doc,
        tinyxml2::XMLElement& parent, const char* elementName) const;

    virtual uint16_t GetDynamicSizeCount() const;
    bool SetDynamicSizeCount(uint16_t dynamicSizeCount);

    virtual std::string GetCodeType() const;
    virtual std::string GetConstructValue() const;
    std::string GetConstructorCodeDefaults(const std::string& varName,
        const std::string& parentRef, size_t tabLevel) const;
    virtual std::string GetValidCondition(const Generator& generator,
        const std::string& name, bool recursive = false) const;
    virtual std::string GetDatabaseLoadCode(const Generator& generator,
        const std::string& name, size_t tabLevel = 1) const;
    virtual std::string GetLoadCode(const Generator& generator,
        const std::string& name, const std::string& stream) const;
    virtual std::string GetSaveCode(const Generator& generator,
        const std::string& name, const std::string& stream) const;
    virtual std::string GetLoadRawCode(const Generator& generator,
        const std::string& name, const std::string& stream) const;
    virtual std::string GetSaveRawCode(const Generator& generator,
        const std::string& name, const std::string& stream) const;
    virtual std::string GetXmlLoadCode(const Generator& generator,
        const std::string& name, const std::string& doc,
        const std::string& node, size_t tabLevel = 1) const;
    virtual std::string GetXmlSaveCode(const Generator& generator,
        const std::string& name, const std::string& doc,
        const std::string& parent, size_t tabLevel = 1,
        const std::string elemName = "member") const;
    virtual std::string GetBindValueCode(const Generator& generator,
        const std::string& name, size_t tabLevel = 1) const;
    virtual std::string GetAccessDeclarations(const Generator& generator,
        const MetaObject& object, const std::string& name,
        size_t tabLevel = 1) const;
    virtual std::string GetAccessFunctions(const Generator& generator,
        const MetaObject& object, const std::string& name) const;

private:
    std::string mReferenceType;
    std::string mNamespace;
    uint16_t mDynamicSizeCount;
    bool mPersistentReference;
    bool mScriptReference;
    bool mNullDefault;

    std::list<std::shared_ptr<MetaVariable>> mDefaultedVariables;
};

} // namespace libobjgen

#endif // LIBOBJGEN_SRC_METAVARIABLEREFERENCE_H
