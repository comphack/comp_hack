/**
 * @file libobjgen/src/MetaVariableString.cpp
 * @ingroup libobjgen
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Meta data for an string based object member variable.
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

#include "MetaVariableString.h"

// libobjgen Includes
#include "Generator.h"
#include "MetaVariableInt.h"

// Standard C++11 Libraries
#include <regex>
#include <sstream>

using namespace libobjgen;

MetaVariableString::MetaVariableString() : MetaVariable(), mSize(0u),
    mRounding(0), mLengthSize(4), mAllowEmpty(true),
    mEncoding(Encoding_t::ENCODING_UTF8)
{
}

MetaVariableString::~MetaVariableString()
{
}

size_t MetaVariableString::GetLengthSize() const
{
    return mLengthSize;
}

void MetaVariableString::SetLengthSize(size_t lengthSize)
{
    mLengthSize = lengthSize;
}

size_t MetaVariableString::GetRounding() const
{
    return mRounding;
}

void MetaVariableString::SetRounding(size_t rounding)
{
    mRounding = rounding;
}

bool MetaVariableString::GetAllowEmpty() const
{
    return mAllowEmpty;
}

void MetaVariableString::SetAllowEmpty(bool allowEmpty)
{
    mAllowEmpty = allowEmpty;
}

MetaVariableString::Encoding_t MetaVariableString::GetEncoding() const
{
    return mEncoding;
}

void MetaVariableString::SetEncoding(Encoding_t encoding)
{
    mEncoding = encoding;
}

std::string MetaVariableString::GetRegularExpression() const
{
    return mRegularExpression;
}

bool MetaVariableString::SetRegularExpression(const std::string& regex)
{
    bool status = false;

    try
    {
        std::regex re(regex);

        status = true;
        mRegularExpression = regex;
    }
    catch(...)
    {
    }

    return status;
}

std::string MetaVariableString::GetDefaultValue() const
{
    return mDefaultValue;
}

void MetaVariableString::SetDefaultValue(const std::string& value)
{
    mDefaultValue = value;
}

size_t MetaVariableString::GetSize() const
{
    return mSize;
}

void MetaVariableString::SetSize(size_t size)
{
    mSize = size;
}

MetaVariable::MetaVariableType_t MetaVariableString::GetMetaType() const
{
    return MetaVariable::MetaVariableType_t::TYPE_STRING;
}

std::string MetaVariableString::GetType() const
{
    return "string";
}

bool MetaVariableString::IsCoreType() const
{
    return true;
}

bool MetaVariableString::IsScriptAccessible() const
{
    return true;
}

bool MetaVariableString::IsValid() const
{
    bool regexOk = true;

    if(!mRegularExpression.empty())
    {
        regexOk = std::regex_match(mDefaultValue,
            std::regex(mRegularExpression));
    }

    return regexOk && (0 == mSize || mSize > mDefaultValue.size());
}

bool MetaVariableString::Load(std::istream& stream)
{
    MetaVariable::Load(stream);

    Generator::LoadString(stream, mDefaultValue);
    Generator::LoadString(stream, mRegularExpression);
    stream.read(reinterpret_cast<char*>(&mAllowEmpty),
        sizeof(mAllowEmpty));
    stream.read(reinterpret_cast<char*>(&mEncoding),
        sizeof(mEncoding));
    stream.read(reinterpret_cast<char*>(&mLengthSize),
        sizeof(mLengthSize));
    stream.read(reinterpret_cast<char*>(&mRounding),
        sizeof(mRounding));
    stream.read(reinterpret_cast<char*>(&mSize),
        sizeof(mSize));

    return stream.good() && IsValid();
}

bool MetaVariableString::Save(std::ostream& stream) const
{
    bool result = false;

    if(IsValid() && MetaVariable::Save(stream))
    {
        Generator::SaveString(stream, mDefaultValue);
        Generator::SaveString(stream, mRegularExpression);
        stream.write(reinterpret_cast<const char*>(&mAllowEmpty),
            sizeof(mAllowEmpty));
        stream.write(reinterpret_cast<const char*>(&mEncoding),
            sizeof(mEncoding));
        stream.write(reinterpret_cast<const char*>(&mLengthSize),
            sizeof(mLengthSize));
        stream.write(reinterpret_cast<const char*>(&mRounding),
            sizeof(mRounding));
        stream.write(reinterpret_cast<const char*>(&mSize),
            sizeof(mSize));

        result = stream.good();
    }

    return result;
}

bool MetaVariableString::Load(const tinyxml2::XMLDocument& doc,
    const tinyxml2::XMLElement& root)
{
    (void)doc;

    bool status = true;

    const char *szDefault = root.Attribute("default");

    if(nullptr != szDefault)
    {
        SetDefaultValue(std::string(szDefault));
    }
    else
    {
        mDefaultValue.clear();
    }

    const char *szAllowEmpty = root.Attribute("empty");

    if(nullptr != szAllowEmpty)
    {
        SetAllowEmpty(Generator::GetXmlAttributeBoolean(szAllowEmpty));
    }
    else
    {
        SetAllowEmpty(true);
    }

    const char *szPattern = root.Attribute("regex");

    if(nullptr != szPattern)
    {
        if(nullptr != szAllowEmpty)
        {
            mError = "Attributes 'regex' and 'empty' are mutually "
                "exclusive.";

            status = false;
        }
        else
        {
            SetRegularExpression(std::string(szPattern));
        }
    }
    else
    {
        mRegularExpression.clear();
    }

    const char *szRounding = root.Attribute("round");

    if(nullptr != szRounding)
    {
        std::string rounding(szRounding);

        if("0" == rounding)
        {
            SetRounding(0);
        }
        else if("2" == rounding)
        {
            SetRounding(2);
        }
        else if("4" == rounding)
        {
            SetRounding(4);
        }
        else
        {
            mError = "The only valid rounding values are 0, 2, and 4.";

            status = false;
        }
    }
    else
    {
        SetRounding(0);
    }

    const char *szLengthSize = root.Attribute("lensz");

    if(nullptr != szLengthSize)
    {
        std::string lengthSize(szLengthSize);

        if("0" == lengthSize)
        {
            SetLengthSize(0);
        }
        else if("1" == lengthSize)
        {
            SetLengthSize(1);
        }
        else if("2" == lengthSize)
        {
            SetLengthSize(2);
        }
        else if("4" == lengthSize)
        {
            SetLengthSize(4);
        }
        else
        {
            mError = "The only valid lensize values are 1, 2, and 4.";

            status = false;
        }
    }
    else
    {
        SetLengthSize(4);
    }

    const char *szLength = root.Attribute("length");

    if(nullptr != szLength)
    {
        if(nullptr != szRounding || nullptr != szLengthSize)
        {
            mError = "Attribute 'length' can't be combined with "
                "'round' or 'lensz'.";

            status = false;
        }
        else
        {
            bool ok = false;

            size_t len = MetaVariableInt<size_t>::StringToValue(
                szLength, &ok);

            if(ok)
            {
                SetSize(len);
            }
            else
            {
                status = false;
            }
        }
    }
    else
    {
        SetSize(0);
    }

    const char *szEncoding = root.Attribute("encoding");

    if(nullptr != szEncoding)
    {
        std::string encoding = std::string(szEncoding);

        std::transform(encoding.begin(), encoding.end(),
            encoding.begin(), ::tolower);

        if("utf8" == encoding)
        {
            SetEncoding(Encoding_t::ENCODING_UTF8);
        }
        else if("cp932" == encoding)
        {
            SetEncoding(Encoding_t::ENCODING_CP932);
        }
        else if("cp1252" == encoding)
        {
            SetEncoding(Encoding_t::ENCODING_CP1252);
        }
        else
        {
            status = false;
        }
    }
    else
    {
        SetEncoding(Encoding_t::ENCODING_UTF8);
    }

    return status && BaseLoad(root) && IsValid();
}

bool MetaVariableString::Save(tinyxml2::XMLDocument& doc,
    tinyxml2::XMLElement& parent, const char* elementName) const
{
    tinyxml2::XMLElement *pVariableElement = doc.NewElement(elementName);
    pVariableElement->SetAttribute("type", GetType().c_str());
    pVariableElement->SetAttribute("name", GetName().c_str());

    if(!GetDefaultValue().empty())
    {
        pVariableElement->SetAttribute("default", GetDefaultValue().c_str());
    }

    if(!GetAllowEmpty())
    {
        pVariableElement->SetAttribute("empty", "false");
    }

    if(!GetRegularExpression().empty())
    {
        pVariableElement->SetAttribute("regex", GetRegularExpression().c_str());
    }

    if(0 != GetRounding() && (2 == GetRounding() || 4 == GetRounding()))
    {
        pVariableElement->SetAttribute("round", std::to_string(
            GetRounding()).c_str());
    }

    if(4 != GetLengthSize() && (0 == GetLengthSize() || 1 == GetLengthSize() || 2 == GetLengthSize()))
    {
        pVariableElement->SetAttribute("lensz", std::to_string(
            GetLengthSize()).c_str());
    }

    if(0 != GetSize())
    {
        pVariableElement->SetAttribute("length", std::to_string(
            GetSize()).c_str());
    }

    switch(GetEncoding())
    {
        case Encoding_t::ENCODING_CP932:
            pVariableElement->SetAttribute("encoding", "cp932");
            break;
        case Encoding_t::ENCODING_CP1252:
            pVariableElement->SetAttribute("encoding", "cp1252");
            break;
        default:
            break;
    }

    parent.InsertEndChild(pVariableElement);

    return BaseSave(*pVariableElement);
}

std::string MetaVariableString::GetCodeType() const
{
    return "libcomp::String";
}

std::string MetaVariableString::GetConstructValue() const
{
    std::string code;

    std::string value = GetDefaultValue();

    if(!value.empty())
    {
        code = Generator::Escape(value);
    }

    return code;
}

std::string MetaVariableString::GetDefaultValueCode() const
{
    return Generator::Escape(mDefaultValue);
}

std::string MetaVariableString::GetValidCondition(const Generator& generator,
    const std::string& name, bool recursive) const
{
    (void)generator;
    (void)recursive;

    std::string regex = GetRegularExpression();

    std::stringstream ss;

    if(0 != mSize)
    {
        if(Encoding_t::ENCODING_UTF8 != mEncoding)
        {
            ss << mSize << " > libcomp::Convert::SizeEncoded("
                << EncodingToComp(mEncoding) << ", " << name << ")";
        }
        else
        {
            ss << mSize << " > " << name << ".Size()";
        }

        if(!mAllowEmpty || !regex.empty())
        {
            ss << " && ";
        }
    }

    if(regex.empty() && !mAllowEmpty)
    {
        ss << "!" << name << ".IsEmpty()";
    }
    else if(!regex.empty())
    {
        ss << "std::regex_match(" << name << ".ToUtf8(), std::regex("
            << Generator::Escape(regex) << "))";
    }

    return ss.str();
}

std::string MetaVariableString::GetLoadCode(const Generator& generator,
    const std::string& name, const std::string& stream) const
{
    (void)generator;

    std::string code;

    if(MetaObject::IsValidIdentifier(name) &&
        MetaObject::IsValidIdentifier(stream))
    {
        if(Encoding_t::ENCODING_UTF8 != mEncoding)
        {
            std::stringstream ss;
            ss << name << " = libcomp::Convert::FromEncoding("
                << EncodingToComp(mEncoding) << ", szValue); ";
            code = ss.str();
        }
        else
        {
            std::stringstream ss;
            ss << name << " = szValue; ";
            code = ss.str();
        }

        std::map<std::string, std::string> replacements;
        replacements["@LENGTH_TYPE@"] = LengthSizeType();
        replacements["@FIXED_LENGTH@"] = std::to_string(mSize + 1);
        replacements["@SET_CODE@"] = code;
        replacements["@STREAM@"] = stream;

        if(0 == mSize)
        {
            if(0 == mLengthSize)
            {
                code = generator.ParseTemplate(0, "VariableStringLoadNull",
                    replacements);
            }
            else
            {
                code = generator.ParseTemplate(0, "VariableStringLoadDynamic",
                    replacements);
            }
        }
        else
        {
            code = generator.ParseTemplate(0, "VariableStringLoadFixed",
                replacements);
        }
    }

    return code;
}

std::string MetaVariableString::GetSaveCode(const Generator& generator,
    const std::string& name, const std::string& stream) const
{
    (void)generator;

    std::string code;

    if(MetaObject::IsValidIdentifier(name) &&
        MetaObject::IsValidIdentifier(stream))
    {
        std::map<std::string, std::string> replacements;
        replacements["@LENGTH_TYPE@"] = LengthSizeType();
        replacements["@FIXED_LENGTH@"] = std::to_string(mSize);
        replacements["@ENCODING@"] = EncodingToComp(mEncoding);
        replacements["@VAR_NAME@"] = name;
        replacements["@STREAM@"] = stream;

        bool dynamicString = (0 == mSize) && (mLengthSize > 0);
        if(dynamicString)
        {
            //We need to get the encoded value first BUT write the length before it
            replacements["@ENCODESTREAM@"] = "encodestream";
        }
        else
        {
            replacements["@ENCODESTREAM@"] = stream + ".stream";
        }

        if(Encoding_t::ENCODING_UTF8 != mEncoding)
        {
            replacements["@ENCODE_CODE@"] = generator.ParseTemplate(1,
                "VariableStringToEncoding", replacements);
        }
        else
        {
            replacements["@ENCODE_CODE@"] = generator.ParseTemplate(1,
                "VariableStringToUnicode", replacements);
        }

        if(0 == mSize)
        {
            if(dynamicString)
            {
                code = generator.ParseTemplate(0, "VariableStringSaveDynamic",
                    replacements);
            }
            else
            {
                code = generator.ParseTemplate(0, "VariableStringSaveNull",
                    replacements);
            }
        }
        else
        {
            code = generator.ParseTemplate(0, "VariableStringSaveFixed",
                replacements);
        }
    }

    return code;
}

std::string MetaVariableString::GetLoadRawCode(const Generator& generator,
    const std::string& name, const std::string& stream) const
{
    (void)generator;

    std::string code;

    if(MetaObject::IsValidIdentifier(name) &&
        MetaObject::IsValidIdentifier(stream))
    {
        if(Encoding_t::ENCODING_UTF8 != mEncoding)
        {
            std::stringstream ss;
            ss << name << " = libcomp::Convert::FromEncoding("
                << EncodingToComp(mEncoding) << ", szValue); ";
            code = ss.str();
        }
        else
        {
            std::stringstream ss;
            ss << name << " = szValue; ";
            code = ss.str();
        }

        std::map<std::string, std::string> replacements;
        replacements["@LENGTH_TYPE@"] = LengthSizeType();
        replacements["@FIXED_LENGTH@"] = std::to_string(mSize + 1);
        replacements["@SET_CODE@"] = code;
        replacements["@STREAM@"] = stream;

        code = generator.ParseTemplate(0, "VariableStringLoadRaw",
            replacements);
    }

    return code;
}

std::string MetaVariableString::GetSaveRawCode(const Generator& generator,
    const std::string& name, const std::string& stream) const
{
    (void)generator;

    std::string code;

    if(MetaObject::IsValidIdentifier(name) &&
        MetaObject::IsValidIdentifier(stream))
    {
        std::map<std::string, std::string> replacements;
        replacements["@LENGTH_TYPE@"] = LengthSizeType();
        replacements["@FIXED_LENGTH@"] = std::to_string(mSize);
        replacements["@ENCODING@"] = EncodingToComp(mEncoding);
        replacements["@VAR_NAME@"] = name;
        replacements["@STREAM@"] = stream;

        if(Encoding_t::ENCODING_UTF8 != mEncoding)
        {
            code = generator.ParseTemplate(0, "VariableStringSaveRaw",
                replacements);
        }
        else
        {
            code = generator.ParseTemplate(0, "VariableStringSaveRawUnicode",
                replacements);
        }
    }

    return code;
}

std::string MetaVariableString::GetXmlLoadCode(const Generator& generator,
    const std::string& name, const std::string& doc,
    const std::string& node, size_t tabLevel) const
{
    (void)name;
    (void)doc;

    std::map<std::string, std::string> replacements;
    replacements["@VAR_CAMELCASE_NAME@"] = generator.GetCapitalName(*this);
    replacements["@VAR_NAME@"] = GetName();
    replacements["@NODE@"] = node;

    return generator.ParseTemplate(tabLevel, "VariableStringXmlLoad",
        replacements);
}

std::string MetaVariableString::GetXmlSaveCode(const Generator& generator,
    const std::string& name, const std::string& doc,
    const std::string& parent, size_t tabLevel, const std::string elemName) const
{
    std::map<std::string, std::string> replacements;
    replacements["@GETTER@"] = GetInternalGetterCode(generator, name);
    replacements["@VAR_NAME@"] = generator.Escape(GetName());
    replacements["@ELEMENT_NAME@"] = generator.Escape(elemName);
    replacements["@PARENT@"] = parent;
    replacements["@DOC@"] = doc;

    return generator.ParseTemplate(tabLevel, "VariableStringXmlSave",
        replacements);
}

std::string MetaVariableString::GetBindValueCode(const Generator& generator,
    const std::string& name, size_t tabLevel) const
{
    std::map<std::string, std::string> replacements;
    replacements["@COLUMN_NAME@"] = generator.Escape(GetName());
    replacements["@VAR_NAME@"] = name;
    replacements["@TYPE@"] = "Text";

    return generator.ParseTemplate(tabLevel, "VariableGetTypeBind",
        replacements);
}

std::string MetaVariableString::GetDatabaseLoadCode(const Generator& generator,
    const std::string& name, size_t tabLevel) const
{
    (void)name;

    std::map<std::string, std::string> replacements;
    replacements["@DATABASE_TYPE@"] = GetCodeType();
    replacements["@COLUMN_NAME@"] = generator.Escape(GetName());
    replacements["@SET_FUNCTION@"] = std::string("Set") +
        generator.GetCapitalName(*this);

    return generator.ParseTemplate(tabLevel, "VariableDatabaseLoad",
        replacements);
}

std::string MetaVariableString::EncodingToString(Encoding_t encoding)
{
    std::string str;

    switch(encoding)
    {
        case Encoding_t::ENCODING_CP932:
            str = "cp932";
            break;
        case Encoding_t::ENCODING_CP1252:
            str = "cp1252";
            break;
        default:
        case Encoding_t::ENCODING_UTF8:
            str = "utf8";
            break;
    }

    return str;
}

std::string MetaVariableString::EncodingToComp(Encoding_t encoding)
{
    std::string str;

    switch(encoding)
    {
        case Encoding_t::ENCODING_CP932:
            str = "libcomp::Convert::ENCODING_CP932";
            break;
        case Encoding_t::ENCODING_CP1252:
            str = "libcomp::Convert::ENCODING_CP1252";
            break;
        default:
        case Encoding_t::ENCODING_UTF8:
            str = "libcomp::Convert::ENCODING_UTF8";
            break;
    }

    return str;
}

std::string MetaVariableString::LengthSizeType() const
{
    std::string lengthSizeType;

    switch(mLengthSize)
    {
        case 1:
            lengthSizeType = "uint8_t";
            break;
        case 2:
            lengthSizeType = "uint16_t";
            break;
        default:
        case 4:
            lengthSizeType = "uint32_t";
            break;
    }

    return lengthSizeType;
}
