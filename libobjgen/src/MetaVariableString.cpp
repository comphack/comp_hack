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

std::string MetaVariableString::GetType() const
{
    return "string";
}

bool MetaVariableString::IsCoreType() const
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

    return regexOk && (0 == mSize || mSize > mDefaultValue.size()) &&
        MetaObject::IsValidIdentifier(GetName());;
}

bool MetaVariableString::IsValid(const void *pData, size_t dataSize) const
{
    (void)pData;
    (void)dataSize;

    /// @todo Fix
    return true;
}

bool MetaVariableString::Load(std::istream& stream)
{
    (void)stream;

    /// @todo Fix
    return true;
}

bool MetaVariableString::Save(std::ostream& stream) const
{
    (void)stream;

    /// @todo Fix
    return true;
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
        std::string allowEmpty(szAllowEmpty);

        std::transform(allowEmpty.begin(), allowEmpty.end(),
            allowEmpty.begin(), ::tolower);

        SetAllowEmpty("1" == allowEmpty || "true" == allowEmpty ||
            "on" == allowEmpty || "yes" == allowEmpty);
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
            mError = "Attributes 'regex' and 'empty' and mutually "
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
    tinyxml2::XMLElement& root) const
{
    (void)doc;
    (void)root;

    /// @todo Fix
    return true;
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

std::string MetaVariableString::GetValidCondition(const std::string& name,
    bool recursive) const
{
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

std::string MetaVariableString::GetLoadCode(const std::string& name,
    const std::string& stream) const
{
    std::string code;

    if(MetaObject::IsValidIdentifier(name) &&
        MetaObject::IsValidIdentifier(stream))
    {
        std::stringstream ss;

        if(0 == mSize)
        {
            if(0 == mLengthSize)
            {
                ss << "( [&]() -> bool { ";
                ss << "std::vector<char> s; ";
                ss << "char c; ";
                ss << "do { ";
                ss << stream << ".stream.read(&c, sizeof(c)); ";
                ss << "if(" << stream << ".stream.good()) { ";
                ss << "s.push_back(c); ";
                ss << "} ";
                ss << "} ";
                ss << "while(0 != c && " << stream << ".stream.good()); ";
                ss << "if(!" << stream << ".stream.good()) { return false; } ";

                if(Encoding_t::ENCODING_UTF8 != mEncoding)
                {
                    ss << name << " = libcomp::Convert::FromEncoding("
                        << EncodingToComp(mEncoding) << ", s); ";
                }
                else
                {
                    ss << name << " = s; ";
                }

                ss << "return " << stream << ".stream.good(); } )()";
            }
            else
            {
#if 1
            // This is the not retarted version.
            ss << "( [&]() -> bool { ";
            ss << LengthSizeType() << " len; ";
            ss << stream << ".stream.read(reinterpret_cast<char*>(&len), "
                << "sizeof(len)); ";
            ss << "if(!" << stream << ".stream.good()) ";
            ss << "{ return false; } ";
            ss << "if(0 == len) ";
            ss << "{ return true; } ";
            ss << "char *szValue = new char[len + 1]; ";
            ss << "szValue[len] = 0; ";
            ss << stream << ".stream.read(szValue, len); ";
            ss << "if(!" << stream << ".stream.good()) " ;
            ss << "{ delete[] szValue; return false; } ";

            if(Encoding_t::ENCODING_UTF8 != mEncoding)
            {
                ss << name << " = libcomp::Convert::FromEncoding("
                    << EncodingToComp(mEncoding) << ", szValue); ";
            }
            else
            {
                ss << name << " = szValue; ";
            }

            ss << "delete[] szValue; ";
            ss << "return " << stream << ".stream.good(); } )()";
#else
            ss << "( [&]() -> bool { ";
            ss << LengthSizeType() << " len; ";
            ss << "if(" << stream << ".dynamicSizes.empty()) ";
            ss << "{ return false; } ";
            ss << "len = static_cast<" << LengthSizeType() << ">("
                << stream << ".dynamicSizes.front()); ";
            ss << stream << ".dynamicSizes.pop_front(); ";
            ss << "if(0 == len) ";
            ss << "{ return true; } ";
            ss << "char *szValue = new char[len + 1]; ";
            ss << "szValue[len] = 0; ";
            ss << stream << ".stream.read(szValue, len); ";
            ss << "if(!" << stream << ".stream.good()) " ;
            ss << "{ delete[] szValue; return false; } ";

            if(Encoding_t::ENCODING_UTF8 != mEncoding)
            {
                ss << name << " = libcomp::Convert::FromEncoding("
                    << EncodingToComp(mEncoding) << ", szValue); ";
            }
            else
            {
                ss << name << " = szValue; ";
            }

            ss << "delete[] szValue; ";
            ss << "return " << stream << ".stream.good(); } )()";
#endif
            }
        }
        else
        {
            ss << "( [&]() -> bool { ";
            ss << "char szValue[" << (mSize + 1) << "]; ";
            ss << "szValue[" << mSize << "] = 0; ";
            ss << stream << ".stream.read(szValue, sizeof(szValue) - 1); ";
            ss << "if(!" << stream << ".stream.good())" ;
            ss << "{ return false; } ";
            
            if(Encoding_t::ENCODING_UTF8 != mEncoding)
            {
                ss << name << " = libcomp::Convert::FromEncoding("
                    << EncodingToComp(mEncoding) << ", szValue); ";
            }
            else
            {
                ss << name << " = szValue; ";
            }

            ss << "return " << stream << ".stream.good(); } )()";
        }

        code = ss.str();
    }

    return code;
}

std::string MetaVariableString::GetSaveCode(const std::string& name,
    const std::string& stream) const
{
    std::string code;

    if(MetaObject::IsValidIdentifier(name) &&
        MetaObject::IsValidIdentifier(stream))
    {
        std::stringstream ss;

        if(0 == mSize)
        {
            /// @todo Fix
        }
        else
        {
            ss << "( [&]() -> bool { ";
            ss << "static const char zero[" << mSize << "] = { 0 }; ";

            if(Encoding_t::ENCODING_UTF8 != mEncoding)
            {
                ss << "std::vector<char> value = libcomp::Convert::ToEncoding("
                    << EncodingToComp(mEncoding) << ", " << name << "); ";
                ss << "if(!value.empty()) { " << stream
                    << ".stream.write(&value[0], "
                    << "static_cast<std::streamsize>(value.size())); ";
            }
            else
            {
                ss << "std::string value = " << name << ".ToUtf8(); ";
                ss << "if(!value.empty()) { " << stream
                    << ".stream.write(value.c_str(), "
                    << "static_cast<std::streamsize>(value.size())); ";
            }

            ss << " } ";
            ss << "if(" << stream << ".stream.good() && " << mSize
                << " != value.size()) { "
                << stream << ".stream.write(zero, static_cast<std::streamsize>("
                << mSize << ") - static_cast<std::streamsize>(value.size())); ";
            ss << " } ";
            ss << "return " << stream << ".stream.good(); } )()";
        }

        code = ss.str();
    }

    return code;
}

std::string MetaVariableString::GetXmlLoadCode(const Generator& generator,
    const std::string& name, const std::string& doc,
    const std::string& root, const std::string& members,
    size_t tabLevel) const
{
    (void)generator;
    (void)name;
    (void)doc;
    (void)root;
    (void)members;

    std::stringstream ss;
    ss << generator.Tab(tabLevel) << "if(status)" << std::endl;
    ss << generator.Tab(tabLevel) << "{" << std::endl;
    ss << generator.Tab(tabLevel + 1) << "std::unordered_map<std::string, "
        "const tinyxml2::XMLElement*>::const_iterator memberIterator = "
        << members << ".find(" << generator.Escape(GetName())
        << ");" << std::endl;
    ss << std::endl;
    ss << generator.Tab(tabLevel + 1) << "if(memberIterator != "
        << members << ".end())" << std::endl;
    ss << generator.Tab(tabLevel + 1) << "{" << std::endl;
    ss << generator.Tab(tabLevel + 2) << "const tinyxml2::XMLElement *pMember"
        << " = memberIterator->second;" << std::endl;
    ss << generator.Tab(tabLevel + 2) << "if(!Set" << generator.GetCapitalName(
        *this) << "(GetXmlText(*pMember)))" << std::endl;
    ss << generator.Tab(tabLevel + 2) << "{" << std::endl;
    ss << generator.Tab(tabLevel + 3) << "status = false;" << std::endl;
    ss << generator.Tab(tabLevel + 2) << "}" << std::endl;
    ss << generator.Tab(tabLevel + 1) << "}" << std::endl;
    ss << generator.Tab(tabLevel) << "}" << std::endl;

    return ss.str();
}

std::string MetaVariableString::GetXmlSaveCode(const Generator& generator,
    const std::string& name, const std::string& doc,
    const std::string& root, size_t tabLevel) const
{
    (void)doc;

    std::stringstream ss;
    ss << generator.Tab(tabLevel) << "{" << std::endl;
    ss << generator.Tab(tabLevel + 1) << "tinyxml2::XMLElement *pMember = "
        "doc.NewElement(\"member\");" << std::endl;
    ss << generator.Tab(tabLevel + 1) << "pMember->SetAttribute(\"name\", "
            << generator.Escape(GetName()) <<  ");" << std::endl;
    ss << generator.Tab(tabLevel + 1) << "tinyxml2::XMLText *pText = "
        "doc.NewText(" << GetInternalGetterCode(generator, name) << ".C());" << std::endl;
    ss << generator.Tab(tabLevel + 1) << "pText->SetCData(true);" << std::endl;
    ss << generator.Tab(tabLevel + 1) << "pMember->InsertEndChild(pText);"
        << std::endl;
    ss << generator.Tab(tabLevel + 1) << root << ".InsertEndChild(pMember);"
        << std::endl;
    ss << generator.Tab(tabLevel) << "}" << std::endl;

    return ss.str();
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
