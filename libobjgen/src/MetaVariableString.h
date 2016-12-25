/**
 * @file libobjgen/src/MetaVariableString.h
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

#ifndef LIBOBJGEN_SRC_METAVARIABLESTRING_H
#define LIBOBJGEN_SRC_METAVARIABLESTRING_H

// libobjgen Includes
#include "MetaVariable.h"

namespace libobjgen
{

class MetaVariableString : public MetaVariable
{
public:
    /**
     * Valid string encodings.
     */
    enum class Encoding_t : int8_t
    {
        ENCODING_UTF8 = 0,
        ENCODING_CP932,
        ENCODING_CP1252,
    };

    MetaVariableString();
    virtual ~MetaVariableString();

    size_t GetLengthSize() const;
    void SetLengthSize(size_t lengthSize);

    size_t GetRounding() const;
    void SetRounding(size_t rounding);

    bool GetAllowEmpty() const;
    void SetAllowEmpty(bool allowEmpty);

    Encoding_t GetEncoding() const;
    void SetEncoding(Encoding_t encoding);

    std::string GetRegularExpression() const;
    bool SetRegularExpression(const std::string& regex);

    std::string GetDefaultValue() const;
    void SetDefaultValue(const std::string& value);

    void SetSize(size_t size);
    virtual size_t GetSize() const;

    virtual MetaVariableType_t GetMetaType() const;

    virtual std::string GetType() const;

    virtual bool IsCoreType() const;
    virtual bool IsScriptAccessible() const;
    virtual bool IsValid() const;

    virtual bool Load(std::istream& stream);
    virtual bool Save(std::ostream& stream) const;

    virtual bool Load(const tinyxml2::XMLDocument& doc,
        const tinyxml2::XMLElement& root);
    virtual bool Save(tinyxml2::XMLDocument& doc,
        tinyxml2::XMLElement& parent, const char* elementName) const;

    virtual std::string GetCodeType() const;
    virtual std::string GetConstructValue() const;
    virtual std::string GetDefaultValueCode() const;
    virtual std::string GetValidCondition(const Generator& generator,
        const std::string& name, bool recursive = false) const;
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
    virtual std::string GetDatabaseLoadCode(const Generator& generator,
        const std::string& name, size_t tabLevel = 1) const;

    static std::string EncodingToString(Encoding_t encoding);
    static std::string EncodingToComp(Encoding_t encoding);

    std::string LengthSizeType() const;

private:
    size_t mSize;
    size_t mRounding;
    size_t mLengthSize;
    bool mAllowEmpty;
    Encoding_t mEncoding;
    std::string mDefaultValue;
    std::string mRegularExpression;
};

} // namespace libobjgen

#endif // LIBOBJGEN_SRC_METAVARIABLESTRING_H
