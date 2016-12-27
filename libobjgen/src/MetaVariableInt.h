/**
 * @file libobjgen/src/MetaVariableInt.h
 * @ingroup libobjgen
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Meta data for an integer based object member variable.
 *
 * This file is part of the COMP_hack Object Generator Library (libobjgen).
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

#ifndef LIBOBJGEN_SRC_METAVARIABLEINT_H
#define LIBOBJGEN_SRC_METAVARIABLEINT_H

// libobjgen Includes
#include "Generator.h"
#include "MetaObject.h"
#include "MetaVariable.h"

// Standard C++11 Includes
#include <iomanip>
#include <limits>
#include <regex>
#include <sstream>

// tinyxml2 Includes
#include "PushIgnore.h"
#include <tinyxml2.h>
#include "PopIgnore.h"

namespace libobjgen
{

template<typename T>
class MetaVariableInt : public MetaVariable
{
private:
    T mDefaultValue;
    T mMinimumValue;
    T mMaximumValue;

public:
    MetaVariableInt() : MetaVariable(), mDefaultValue(0),
        mMinimumValue(std::numeric_limits<T>::lowest()),
        mMaximumValue(std::numeric_limits<T>::max())
    {
    }

    virtual ~MetaVariableInt()
    {
    }

    virtual size_t GetSize() const
    {
        return sizeof(T);
    }

    // You must specialize this.
    virtual MetaVariableType_t GetMetaType() const;

    virtual std::string GetType() const
    {
        std::stringstream ss;

        if(std::numeric_limits<T>::is_integer)
        {
            if(std::numeric_limits<T>::is_signed)
            {
                ss << "s";
            }
            else
            {
                ss << "u";
            }
        }
        else
        {
            ss << "f";
        }

        size_t bitSize = 8u * GetSize();

        ss << bitSize;

        return ss.str();
    }

    T GetDefaultValue() const
    {
        return mDefaultValue;
    }

    void SetDefaultValue(T value)
    {
        mDefaultValue = value;
    }

    T GetMinimumValue() const
    {
        return mMinimumValue;
    }

    void SetMinimumValue(T value)
    {
        mMinimumValue = value;
    }

    T GetMaximumValue() const
    {
        return mMaximumValue;
    }

    void SetMaximumValue(T value)
    {
        mMaximumValue = value;
    }

    static T StringToValue(const std::string& str, bool *pOK = nullptr)
    {
        T value = 0;
        bool ok = true;
        bool decimal = false;
        int base = 10;
        std::smatch match;
        std::string s = str;

        if(std::regex_match(s, match, std::regex(
            "^([+-])?0x([0-9a-fA-F]+)$")))
        {
            base = 16;
            s = std::string(match[1]) + std::string(match[2]);
        }
        else if(std::regex_match(s, match, std::regex(
            "^([+-])?0([0-7]+)$")))
        {
            base = 8;
            s = std::string(match[1]) + std::string(match[2]);
        }
        else if(!std::numeric_limits<T>::is_integer &&
            std::regex_match(s, match, std::regex(
            "^[-+]?([0-9]*.[0-9]+|[0-9]+)")))
        {
            decimal = true;
        }
        else if(!std::regex_match(s, match, std::regex(
            "^[+-]?(([1-9][0-9]*)|0)$")))
        {
            ok = false;
        }

        if(ok)
        {
            if(decimal)
            {
                double temp;

                std::stringstream ss(s);
                ss >> std::setbase(base) >> temp;

                if (ss && static_cast<T>(temp) >=
                    std::numeric_limits<T>::lowest() &&
                    static_cast<T>(temp) <=
                    std::numeric_limits<T>::max())
                {
                    value = static_cast<T>(temp);
                }
                else
                {
                    ok = false;
                }
            }
            else if(!std::numeric_limits<T>::is_integer ||
                std::numeric_limits<T>::is_signed)
            {
                int64_t temp;

                std::stringstream ss(s);
                ss >> std::setbase(base) >> temp;

                if(ss && static_cast<T>(temp) >=
                    std::numeric_limits<T>::lowest() &&
                    static_cast<T>(temp) <=
                    std::numeric_limits<T>::max())
                {
                    value = static_cast<T>(temp);
                }
                else
                {
                    ok = false;
                }
            }
            else
            {
                uint64_t temp;

                std::stringstream ss(s);
                ss >> std::setbase(base) >> temp;

                if(ss && static_cast<T>(temp) >=
                    std::numeric_limits<T>::lowest() &&
                    static_cast<T>(temp) <=
                    std::numeric_limits<T>::max())
                {
                    value = static_cast<T>(temp);
                }
                else
                {
                    ok = false;
                }
            }
        }

        if(nullptr != pOK)
        {
            *pOK = ok;
        }

        return value;
    }

    virtual bool IsCoreType() const
    {
        return true;
    }

    virtual bool IsScriptAccessible() const
    {
        return true;
    }

    virtual bool IsValid() const
    {
        return mMinimumValue <= mMaximumValue &&
            mMinimumValue <= mDefaultValue &&
            mMaximumValue >= mDefaultValue;
    }

    virtual bool Load(std::istream& stream)
    {
        MetaVariable::Load(stream);

        stream.read(reinterpret_cast<char*>(&mDefaultValue),
            sizeof(mDefaultValue));
        stream.read(reinterpret_cast<char*>(&mMinimumValue),
            sizeof(mMinimumValue));
        stream.read(reinterpret_cast<char*>(&mMaximumValue),
            sizeof(mMaximumValue));

        return stream.good() && IsValid();
    }

    virtual bool Save(std::ostream& stream) const
    {
        bool result = false;

        if(IsValid() && MetaVariable::Save(stream))
        {
            stream.write(reinterpret_cast<const char*>(&mDefaultValue),
                sizeof(mDefaultValue));
            stream.write(reinterpret_cast<const char*>(&mMinimumValue),
                sizeof(mMinimumValue));
            stream.write(reinterpret_cast<const char*>(&mMaximumValue),
                sizeof(mMaximumValue));

            result = stream.good();
        }

        return result;
    }

    virtual bool Load(const tinyxml2::XMLDocument& doc,
        const tinyxml2::XMLElement& root)
    {
        (void)doc;

        /*
        const char *szType = root.Attribute("type");

        // This check does not work for an alias. For example:
        // float, f32, single
        if(nullptr == szType || szType != GetType())
        {
            std::stringstream ss;
            ss << "Expected type '" << GetType() << "' but got type '"
                << szType << "'.";

            mError = ss.str();

            return false;
        }
        */

        const char *szDefault = root.Attribute("default");

        if(nullptr != szDefault)
        {
            bool ok = false;

            T value = StringToValue(szDefault, &ok);

            if(!ok)
            {
                std::stringstream ss;
                ss << "Invalid default value '" << szDefault << "'.";

                mError = ss.str();

                return false;
            }

            SetDefaultValue(value);
        }

        const char *szMinimum = root.Attribute("min");

        if(nullptr != szMinimum)
        {
            bool ok = false;

            T value = StringToValue(szMinimum, &ok);

            if(!ok)
            {
                std::stringstream ss;
                ss << "Invalid minimum value '" << szMinimum << "'.";

                mError = ss.str();

                return false;
            }

            SetMinimumValue(value);
        }

        const char *szMaximum = root.Attribute("max");

        if(nullptr != szMaximum)
        {
            bool ok = false;

            T value = StringToValue(szMaximum, &ok);

            if(!ok)
            {
                std::stringstream ss;
                ss << "Invalid maximum value '" << szMaximum << "'.";

                mError = ss.str();

                return false;
            }

            SetMaximumValue(value);
        }

        return BaseLoad(root);
    }

    virtual bool Save(tinyxml2::XMLDocument& doc,
        tinyxml2::XMLElement& parent, const char* elementName) const
    {
        tinyxml2::XMLElement *pVariableElement = doc.NewElement(elementName);
        pVariableElement->SetAttribute("type", GetType().c_str());
        pVariableElement->SetAttribute("name", GetName().c_str());

        if(0 != GetDefaultValue())
        {
            if(std::numeric_limits<T>::is_integer)
            {
                pVariableElement->SetAttribute("default", std::to_string(
                    GetDefaultValue()).c_str());
            }
            else
            {
                std::stringstream ss;
                ss << GetDefaultValue();

                pVariableElement->SetAttribute("default", ss.str().c_str());
            }
        }

        if(std::numeric_limits<T>::lowest() != GetMinimumValue())
        {
            if(std::numeric_limits<T>::is_integer)
            {
                pVariableElement->SetAttribute("min", std::to_string(
                    GetMinimumValue()).c_str());
            }
            else
            {
                std::stringstream ss;
                ss << GetMinimumValue();

                pVariableElement->SetAttribute("min", ss.str().c_str());
            }
        }

        if(std::numeric_limits<T>::max() != GetMaximumValue())
        {
            if(std::numeric_limits<T>::is_integer)
            {
                pVariableElement->SetAttribute("max", std::to_string(
                    GetMaximumValue()).c_str());
            }
            else
            {
                std::stringstream ss;
                ss << GetMaximumValue();

                pVariableElement->SetAttribute("max", ss.str().c_str());
            }
        }

        parent.InsertEndChild(pVariableElement);

        return BaseSave(*pVariableElement);
    }

    virtual std::string GetCodeType() const
    {
        std::stringstream ss;

        if(std::numeric_limits<T>::is_integer)
        {
            if(!std::numeric_limits<T>::is_signed)
            {
                ss << "u";
            }

            size_t bitSize = 8u * sizeof(T);

            ss << "int" << bitSize << "_t";
        }
        else if(sizeof(float) == sizeof(T))
        {
            ss << "float";
        }
        else
        {
            ss << "double";
        }

        return ss.str();
    }

    virtual std::string GetConstructValue() const
    {
        std::string value;

        if(std::numeric_limits<T>::is_integer)
        {
            value = std::to_string(mDefaultValue);
        }
        else
        {
            value = GetDefaultValueCode();
        }

        return value;
    }

    virtual std::string GetArgumentType() const
    {
        return GetCodeType();
    }

    virtual std::string GetDefaultValueCode() const
    {
        std::stringstream ss;
        ss << mDefaultValue;
        return ss.str();
    }

    virtual std::string GetValidCondition(const Generator& generator,
        const std::string& name, bool recursive = false) const
    {
        (void)generator;
        (void)recursive;

        std::string minimumCondition;
        std::string maximumCondition;
        std::string condition;

        if(MetaObject::IsValidIdentifier(name))
        {
            if(std::numeric_limits<T>::lowest() != mMinimumValue)
            {
                std::stringstream ss;

                if(std::numeric_limits<T>::is_integer)
                {
                    ss << std::to_string(mMinimumValue);
                }
                else
                {
                    ss << mMinimumValue;
                }

                ss << " <= " << name;

                minimumCondition = ss.str();
            }

            if(std::numeric_limits<T>::max() != mMaximumValue)
            {
                std::stringstream ss;

                if(std::numeric_limits<T>::is_integer)
                {
                    ss << std::to_string(mMaximumValue);
                }
                else
                {
                    ss << mMaximumValue;
                }

                ss << " >= " << name;

                maximumCondition = ss.str();
            }

            if(!minimumCondition.empty() && !maximumCondition.empty())
            {
                std::stringstream ss;
                ss << minimumCondition << " && " << maximumCondition;

                condition = ss.str();
            }
            else if(!minimumCondition.empty())
            {
                condition = minimumCondition;
            }
            else if(!maximumCondition.empty())
            {
                condition = maximumCondition;
            }
        }

        return condition;
    }

    virtual std::string GetLoadCode(const Generator& generator,
        const std::string& name, const std::string& stream) const
    {
        return GetLoadRawCode(generator, name, stream + std::string(".stream"));
    }

    virtual std::string GetSaveCode(const Generator& generator,
        const std::string& name, const std::string& stream) const
    {
        return GetSaveRawCode(generator, name, stream + std::string(".stream"));
    }

    virtual std::string GetLoadRawCode(const Generator& generator,
        const std::string& name, const std::string& stream) const
    {
        (void)generator;

        std::string code;

        if(MetaObject::IsValidIdentifier(name))
        {
            std::map<std::string, std::string> replacements;
            replacements["@VAR_NAME@"] = name;
            replacements["@STREAM@"] = stream;

            code = generator.ParseTemplate(0, "VariableIntLoad",
                replacements);
        }

        return code;
    }

    virtual std::string GetSaveRawCode(const Generator& generator,
        const std::string& name, const std::string& stream) const
    {
        (void)generator;

        std::string code;

        if(MetaObject::IsValidIdentifier(name))
        {
            std::map<std::string, std::string> replacements;
            replacements["@VAR_NAME@"] = name;
            replacements["@STREAM@"] = stream;

            code = generator.ParseTemplate(0, "VariableIntSave",
                replacements);
        }

        return code;
    }

    virtual std::string GetXmlLoadCode(const Generator& generator,
        const std::string& name, const std::string& doc,
        const std::string& node, size_t tabLevel = 1) const
    {
        (void)name;
        (void)doc;

        std::map<std::string, std::string> replacements;
        replacements["@VAR_CAMELCASE_NAME@"] = generator.GetCapitalName(*this);
        replacements["@VAR_CODE_TYPE@"] = GetCodeType();
        replacements["@NODE@"] = node;

        return generator.ParseTemplate(tabLevel, "VariableIntXmlLoad",
            replacements);
    }

    virtual std::string GetXmlSaveCode(const Generator& generator,
        const std::string& name, const std::string& doc,
        const std::string& parent, size_t tabLevel = 1,
        const std::string elemName = "member") const
    {
        (void)doc;

        std::map<std::string, std::string> replacements;
        replacements["@VAR_NAME@"] = generator.Escape(GetName());
        replacements["@ELEMENT_NAME@"] = generator.Escape(elemName);
        replacements["@GETTER@"] = GetInternalGetterCode(generator, name);
        replacements["@PARENT@"] = parent;

        return generator.ParseTemplate(tabLevel, "VariableIntXmlSave",
            replacements);
    }

    virtual std::string GetBindValueCode(const Generator& generator,
        const std::string& name, size_t tabLevel = 1) const
    {
        std::string bindType;
        std::string castType;
        bool cast = false;

        if(std::numeric_limits<T>::is_integer)
        {
            if(sizeof(T) < sizeof(int32_t) || (sizeof(T) == sizeof(int32_t) &&
                std::numeric_limits<T>::is_signed))
            {
                bindType = "Int";
                castType = "int32_t";
                cast = sizeof(T) != sizeof(int32_t);
            }
            else
            {
                bindType = "BigInt";
                castType = "int64_t";
                cast = sizeof(T) != sizeof(int64_t);
            }
        }
        else if(typeid(float) == typeid(T))
        {
            bindType = "Float";
        }
        else if(typeid(double) == typeid(T))
        {
            bindType = "Double";
        }
        else
        {
            // Use the default binary blob.
            return MetaVariable::GetBindValueCode(generator, name, tabLevel);
        }

        std::map<std::string, std::string> replacements;
        replacements["@COLUMN_NAME@"] = generator.Escape(GetName());
        replacements["@VAR_NAME@"] = name;
        replacements["@TYPE@"] = bindType;
        replacements["@CAST@"] = castType;

        if(cast)
        {
            return generator.ParseTemplate(tabLevel, "VariableGetCastBind",
                replacements);
        }
        else
        {
            return generator.ParseTemplate(tabLevel, "VariableGetTypeBind",
                replacements);
        }
    }

    std::string GetDatabaseLoadCode(const Generator& generator,
        const std::string& name, size_t tabLevel) const
    {
        std::string castType = GetCodeType();
        std::string bindType;

        if(std::numeric_limits<T>::is_integer)
        {
            if(sizeof(T) < sizeof(int32_t) || (sizeof(T) == sizeof(int32_t) &&
                std::numeric_limits<T>::is_signed))
            {
                bindType = "int32_t";
            }
            else
            {
                bindType = "int64_t";
            }
        }
        else if(typeid(float) == typeid(T))
        {
            bindType = "float";
        }
        else if(typeid(double) == typeid(T))
        {
            bindType = "double";
        }
        else
        {
            // Use the default binary blob.
            return MetaVariable::GetDatabaseLoadCode(generator, name, tabLevel);
        }

        std::map<std::string, std::string> replacements;
        replacements["@DATABASE_TYPE@"] = bindType;
        replacements["@COLUMN_NAME@"] = generator.Escape(GetName());
        replacements["@SET_FUNCTION@"] = std::string("Set") +
            generator.GetCapitalName(*this);
        replacements["@VAR_TYPE@"] = castType;

        bool cast = (bindType != castType);

        if(cast)
        {
            return generator.ParseTemplate(tabLevel, "VariableDatabaseCastLoad",
                replacements);
        }
        else
        {
            return generator.ParseTemplate(tabLevel, "VariableDatabaseLoad",
                replacements);
        }
    }
};

} // namespace libobjgen

#endif // LIBOBJGEN_SRC_METAVARIABLEINT_H
