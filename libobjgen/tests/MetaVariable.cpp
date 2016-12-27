/**
 * @file libobjgen/tests/MetaVariable.cpp
 * @ingroup libobjgen
 *
 * @author HACKfrost
 *
 * @brief Test the various MetaVariable classes.
 *
 * This file is part of the COMP_hack Object Generator Library (libobjgen).
 *
 * Copyright (C) 2014-2016 COMP_hack Team <compomega@tutanota.com>
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

#include <PushIgnore.h>
#include <gtest/gtest.h>
#include <PopIgnore.h>

// libobjgen Includes
#include <MetaVariable.h>
#include <MetaVariableArray.h>
#include <MetaVariableBool.h>
#include <MetaVariableEnum.h>
#include <MetaVariableInt.h>
#include <MetaVariableList.h>
#include <MetaVariableMap.h>
#include <MetaVariableReference.h>
#include <MetaVariableString.h>

// Standard C++11 Includes
#include <cmath>
#include <string>
#include <sstream>
#include <iostream>

using namespace libobjgen;

TEST(MetaVariableType, Array)
{
    auto elementType = std::shared_ptr<MetaVariableInt<uint8_t>>(
        new MetaVariableInt<uint8_t>);
    elementType->SetDefaultValue(5);

    ASSERT_EQ(5, elementType->GetDefaultValue())
        << "Failed to set/retrieve element default value.";

    MetaVariableArray var(elementType);
    var.SetName("ARRAY");

    ASSERT_FALSE(var.IsValid())
        << "Checking validity without setting element count.";

    var.SetElementCount(3);

    ASSERT_EQ(MetaVariable::MetaVariableType_t::TYPE_ARRAY, var.GetMetaType());
    ASSERT_EQ(MetaVariable::MetaVariableType_t::TYPE_U8,
        var.GetElementType()->GetMetaType());
    ASSERT_TRUE(var.IsValid());
    ASSERT_EQ("ARRAY", var.GetName());
    ASSERT_EQ(elementType->GetSize() * 3, var.GetSize());

    ASSERT_EQ(elementType, var.GetElementType());
    ASSERT_EQ(5, std::static_pointer_cast<MetaVariableInt<uint8_t>>(
        var.GetElementType())->GetDefaultValue());
    ASSERT_EQ(3, var.GetElementCount());

    //Make a copy (via stream) and compare
    std::stringstream ss;
    ASSERT_TRUE(var.Save(ss));

    MetaVariableArray copy(elementType);
    ASSERT_TRUE(copy.Load(ss));

    ASSERT_EQ(var.GetName(), copy.GetName());
    ASSERT_EQ(std::static_pointer_cast<MetaVariableInt<uint8_t>>(
        var.GetElementType())->GetDefaultValue(),
        std::static_pointer_cast<MetaVariableInt<uint8_t>>(
        copy.GetElementType())->GetDefaultValue());
    ASSERT_EQ(var.GetElementCount(), copy.GetElementCount());

    //Make a copy (via XML) and compare
    tinyxml2::XMLDocument doc;
    doc.Parse("<def></def>");

    auto root = doc.RootElement();
    ASSERT_TRUE(var.Save(doc, *root, "var"));

    ASSERT_TRUE(copy.Load(doc, *root->FirstChildElement()));

    ASSERT_EQ(var.GetName(), copy.GetName());
    ASSERT_EQ(std::static_pointer_cast<MetaVariableInt<uint8_t>>(
        var.GetElementType())->GetDefaultValue(),
        std::static_pointer_cast<MetaVariableInt<uint8_t>>(
            copy.GetElementType())->GetDefaultValue());
    ASSERT_EQ(var.GetElementCount(), copy.GetElementCount());
}

TEST(MetaVariableType, Bool)
{
    MetaVariableBool var;
    var.SetName("BOOLEAN");
    var.SetDefaultValue(true);

    ASSERT_EQ(MetaVariable::MetaVariableType_t::TYPE_BOOL, var.GetMetaType());
    ASSERT_TRUE(var.IsValid());
    ASSERT_EQ("BOOLEAN", var.GetName());
    ASSERT_EQ(sizeof(bool), var.GetSize());

    ASSERT_EQ(true, var.GetDefaultValue());

    //Make a copy (via stream) and compare
    std::stringstream ss;
    ASSERT_TRUE(var.Save(ss));

    MetaVariableBool copy;
    ASSERT_TRUE(copy.Load(ss));

    ASSERT_EQ(var.GetName(), copy.GetName());
    ASSERT_EQ(var.GetDefaultValue(), copy.GetDefaultValue());

    //Make a copy (via XML) and compare
    tinyxml2::XMLDocument doc;
    doc.Parse("<def></def>");

    auto root = doc.RootElement();
    ASSERT_TRUE(var.Save(doc, *root, "var"));

    ASSERT_TRUE(copy.Load(doc, *root->FirstChildElement()));

    ASSERT_EQ(var.GetName(), copy.GetName());
    ASSERT_EQ(var.GetDefaultValue(), copy.GetDefaultValue());
}

TEST(MetaVariableType, Enum)
{
    MetaVariableEnum var;
    var.SetName("ENUM");

    ASSERT_FALSE(var.IsValid())
        << "Checking validity with missing requirements.";

    std::vector<std::string> values = { "VALUE_1", "VALUE_2", "VALUE_1" };
    ASSERT_FALSE(var.SetValues(values))
        << "Setting invalid values containing an duplicate.";

    values = { "VALUE_1", "VALUE_2", "VALUE_3" };
    ASSERT_TRUE(var.SetValues(values))
        << "Setting valid values.";

    var.SetDefaultValue("VALUE_3");
    var.SetTypePrefix("Testing");

    ASSERT_FALSE(var.SetSizeType(128))
        << "Setting an invalid size type.";

    ASSERT_TRUE(var.SetSizeType(32))
        << "Setting a valid size type.";

    ASSERT_EQ(MetaVariable::MetaVariableType_t::TYPE_ENUM, var.GetMetaType());
    ASSERT_TRUE(var.IsValid());
    ASSERT_EQ("ENUM", var.GetName());
    ASSERT_EQ(sizeof(uint32_t), var.GetSize());

    ASSERT_EQ("VALUE_3", var.GetDefaultValue());
    ASSERT_EQ("Testing", var.GetTypePrefix());
    ASSERT_EQ(32, var.GetSizeType());

    //Make a copy (via stream) and compare
    std::stringstream ss;
    ASSERT_TRUE(var.Save(ss));

    MetaVariableEnum copy;
    ASSERT_TRUE(copy.Load(ss));

    ASSERT_EQ(var.GetName(), copy.GetName());
    ASSERT_EQ(var.GetDefaultValue(), copy.GetDefaultValue());
    ASSERT_EQ(var.GetTypePrefix(), copy.GetTypePrefix());
    ASSERT_EQ(var.GetSizeType(), copy.GetSizeType());

    //Make a copy (via XML) and compare
    tinyxml2::XMLDocument doc;
    doc.Parse("<def></def>");

    auto root = doc.RootElement();
    ASSERT_TRUE(var.Save(doc, *root, "var"));

    ASSERT_TRUE(copy.Load(doc, *root->FirstChildElement()));

    ASSERT_EQ(var.GetName(), copy.GetName());
    ASSERT_EQ(var.GetDefaultValue(), copy.GetDefaultValue());
    ASSERT_EQ(var.GetTypePrefix(), copy.GetTypePrefix());
    ASSERT_EQ(var.GetSizeType(), copy.GetSizeType());
}

TEST(MetaVariableType, Int_s8)
{
    MetaVariableInt<int8_t> var;
    var.SetName("S8");
    var.SetMinimumValue(-4);
    var.SetMaximumValue(4);

    var.SetDefaultValue(5);

    ASSERT_FALSE(var.IsValid())
        << "Setting a value above of the valid value range.";

    var.SetDefaultValue(-5);
    ASSERT_FALSE(var.IsValid())
        << "Setting a value below of the valid value range.";

    var.SetDefaultValue(-1);
    ASSERT_EQ(-1, var.GetDefaultValue());

    var.SetDefaultValue(3);
    ASSERT_EQ(3, var.GetDefaultValue());

    ASSERT_EQ(MetaVariable::MetaVariableType_t::TYPE_S8, var.GetMetaType());
    ASSERT_TRUE(var.IsValid());
    ASSERT_EQ("S8", var.GetName());
    ASSERT_EQ(sizeof(int8_t), var.GetSize());

    //Make a copy (via stream) and compare
    std::stringstream ss;
    ASSERT_TRUE(var.Save(ss));

    MetaVariableInt<int8_t> copy;
    ASSERT_TRUE(copy.Load(ss));

    ASSERT_EQ(var.GetName(), copy.GetName());
    ASSERT_EQ(var.GetDefaultValue(), copy.GetDefaultValue());

    //Make a copy (via XML) and compare
    tinyxml2::XMLDocument doc;
    doc.Parse("<def></def>");

    auto root = doc.RootElement();
    ASSERT_TRUE(var.Save(doc, *root, "var"));

    ASSERT_TRUE(copy.Load(doc, *root->FirstChildElement()));

    ASSERT_EQ(var.GetName(), copy.GetName());
    ASSERT_EQ(var.GetDefaultValue(), copy.GetDefaultValue());
}

TEST(MetaVariableType, Int_u8)
{
    MetaVariableInt<uint8_t> var;
    var.SetName("U8");
    var.SetMinimumValue(1);
    var.SetMaximumValue(20);

    var.SetDefaultValue(0);

    ASSERT_FALSE(var.IsValid())
        << "Setting a value above of the valid value range.";

    var.SetDefaultValue(25);
    ASSERT_FALSE(var.IsValid())
        << "Setting a value below of the valid value range.";

    var.SetDefaultValue(15);
    ASSERT_EQ(15, var.GetDefaultValue());

    var.SetDefaultValue(3);
    ASSERT_EQ(3, var.GetDefaultValue());

    ASSERT_EQ(MetaVariable::MetaVariableType_t::TYPE_U8, var.GetMetaType());
    ASSERT_TRUE(var.IsValid());
    ASSERT_EQ("U8", var.GetName());
    ASSERT_EQ(sizeof(uint8_t), var.GetSize());

    //Make a copy (via stream) and compare
    std::stringstream ss;
    ASSERT_TRUE(var.Save(ss));

    MetaVariableInt<uint8_t> copy;
    ASSERT_TRUE(copy.Load(ss));

    ASSERT_EQ(var.GetName(), copy.GetName());
    ASSERT_EQ(var.GetDefaultValue(), copy.GetDefaultValue());

    //Make a copy (via XML) and compare
    tinyxml2::XMLDocument doc;
    doc.Parse("<def></def>");

    auto root = doc.RootElement();
    ASSERT_TRUE(var.Save(doc, *root, "var"));

    ASSERT_TRUE(copy.Load(doc, *root->FirstChildElement()));

    ASSERT_EQ(var.GetName(), copy.GetName());
    ASSERT_EQ(var.GetDefaultValue(), copy.GetDefaultValue());
}

TEST(MetaVariableType, Int_s16)
{
    MetaVariableInt<int16_t> var;
    var.SetName("S16");
    var.SetMinimumValue(-4);
    var.SetMaximumValue(4);

    var.SetDefaultValue(5);

    ASSERT_FALSE(var.IsValid())
        << "Setting a value above of the valid value range.";

    var.SetDefaultValue(-5);
    ASSERT_FALSE(var.IsValid())
        << "Setting a value below of the valid value range.";

    var.SetDefaultValue(-1);
    ASSERT_EQ(-1, var.GetDefaultValue());

    var.SetDefaultValue(3);
    ASSERT_EQ(3, var.GetDefaultValue());

    ASSERT_EQ(MetaVariable::MetaVariableType_t::TYPE_S16, var.GetMetaType());
    ASSERT_TRUE(var.IsValid());
    ASSERT_EQ("S16", var.GetName());
    ASSERT_EQ(sizeof(int16_t), var.GetSize());

    //Make a copy (via stream) and compare
    std::stringstream ss;
    ASSERT_TRUE(var.Save(ss));

    MetaVariableInt<int16_t> copy;
    ASSERT_TRUE(copy.Load(ss));

    ASSERT_EQ(var.GetName(), copy.GetName());
    ASSERT_EQ(var.GetDefaultValue(), copy.GetDefaultValue());

    //Make a copy (via XML) and compare
    tinyxml2::XMLDocument doc;
    doc.Parse("<def></def>");

    auto root = doc.RootElement();
    ASSERT_TRUE(var.Save(doc, *root, "var"));

    ASSERT_TRUE(copy.Load(doc, *root->FirstChildElement()));

    ASSERT_EQ(var.GetName(), copy.GetName());
    ASSERT_EQ(var.GetDefaultValue(), copy.GetDefaultValue());
}

TEST(MetaVariableType, Int_u16)
{
    MetaVariableInt<uint16_t> var;
    var.SetName("U16");
    var.SetMinimumValue(1);
    var.SetMaximumValue(20);

    var.SetDefaultValue(0);

    ASSERT_FALSE(var.IsValid())
        << "Setting a value above of the valid value range.";

    var.SetDefaultValue(25);
    ASSERT_FALSE(var.IsValid())
        << "Setting a value below of the valid value range.";

    var.SetDefaultValue(15);
    ASSERT_EQ(15, var.GetDefaultValue());

    var.SetDefaultValue(3);
    ASSERT_EQ(3, var.GetDefaultValue());

    ASSERT_EQ(MetaVariable::MetaVariableType_t::TYPE_U16, var.GetMetaType());
    ASSERT_TRUE(var.IsValid());
    ASSERT_EQ("U16", var.GetName());
    ASSERT_EQ(sizeof(uint16_t), var.GetSize());

    //Make a copy (via stream) and compare
    std::stringstream ss;
    ASSERT_TRUE(var.Save(ss));

    MetaVariableInt<uint16_t> copy;
    ASSERT_TRUE(copy.Load(ss));

    ASSERT_EQ(var.GetName(), copy.GetName());
    ASSERT_EQ(var.GetDefaultValue(), copy.GetDefaultValue());

    //Make a copy (via XML) and compare
    tinyxml2::XMLDocument doc;
    doc.Parse("<def></def>");

    auto root = doc.RootElement();
    ASSERT_TRUE(var.Save(doc, *root, "var"));

    ASSERT_TRUE(copy.Load(doc, *root->FirstChildElement()));

    ASSERT_EQ(var.GetName(), copy.GetName());
    ASSERT_EQ(var.GetDefaultValue(), copy.GetDefaultValue());
}

TEST(MetaVariableType, Int_s32)
{
    MetaVariableInt<int32_t> var;
    var.SetName("S32");
    var.SetMinimumValue(-4);
    var.SetMaximumValue(4);

    var.SetDefaultValue(5);

    ASSERT_FALSE(var.IsValid())
        << "Setting a value above of the valid value range.";

    var.SetDefaultValue(-5);
    ASSERT_FALSE(var.IsValid())
        << "Setting a value below of the valid value range.";

    var.SetDefaultValue(-1);
    ASSERT_EQ(-1, var.GetDefaultValue());

    var.SetDefaultValue(3);
    ASSERT_EQ(3, var.GetDefaultValue());

    ASSERT_EQ(MetaVariable::MetaVariableType_t::TYPE_S32, var.GetMetaType());
    ASSERT_TRUE(var.IsValid());
    ASSERT_EQ("S32", var.GetName());
    ASSERT_EQ(sizeof(int32_t), var.GetSize());

    //Make a copy (via stream) and compare
    std::stringstream ss;
    ASSERT_TRUE(var.Save(ss));

    MetaVariableInt<int32_t> copy;
    ASSERT_TRUE(copy.Load(ss));

    ASSERT_EQ(var.GetName(), copy.GetName());
    ASSERT_EQ(var.GetDefaultValue(), copy.GetDefaultValue());

    //Make a copy (via XML) and compare
    tinyxml2::XMLDocument doc;
    doc.Parse("<def></def>");

    auto root = doc.RootElement();
    ASSERT_TRUE(var.Save(doc, *root, "var"));

    ASSERT_TRUE(copy.Load(doc, *root->FirstChildElement()));

    ASSERT_EQ(var.GetName(), copy.GetName());
    ASSERT_EQ(var.GetDefaultValue(), copy.GetDefaultValue());
}

TEST(MetaVariableType, Int_u32)
{
    MetaVariableInt<uint32_t> var;
    var.SetName("U32");
    var.SetMinimumValue(1);
    var.SetMaximumValue(20);

    var.SetDefaultValue(0);

    ASSERT_FALSE(var.IsValid())
        << "Setting a value above of the valid value range.";

    var.SetDefaultValue(25);
    ASSERT_FALSE(var.IsValid())
        << "Setting a value below of the valid value range.";

    var.SetDefaultValue(15);
    ASSERT_EQ(15, var.GetDefaultValue());

    var.SetDefaultValue(3);
    ASSERT_EQ(3, var.GetDefaultValue());

    ASSERT_EQ(MetaVariable::MetaVariableType_t::TYPE_U32, var.GetMetaType());
    ASSERT_TRUE(var.IsValid());
    ASSERT_EQ("U32", var.GetName());
    ASSERT_EQ(sizeof(uint32_t), var.GetSize());

    //Make a copy (via stream) and compare
    std::stringstream ss;
    ASSERT_TRUE(var.Save(ss));

    MetaVariableInt<uint32_t> copy;
    ASSERT_TRUE(copy.Load(ss));

    ASSERT_EQ(var.GetName(), copy.GetName());
    ASSERT_EQ(var.GetDefaultValue(), copy.GetDefaultValue());

    //Make a copy (via XML) and compare
    tinyxml2::XMLDocument doc;
    doc.Parse("<def></def>");

    auto root = doc.RootElement();
    ASSERT_TRUE(var.Save(doc, *root, "var"));

    ASSERT_TRUE(copy.Load(doc, *root->FirstChildElement()));

    ASSERT_EQ(var.GetName(), copy.GetName());
    ASSERT_EQ(var.GetDefaultValue(), copy.GetDefaultValue());
}

TEST(MetaVariableType, Int_s64)
{
    MetaVariableInt<int64_t> var;
    var.SetName("S64");
    var.SetMinimumValue(-4);
    var.SetMaximumValue(4);

    var.SetDefaultValue(5);

    ASSERT_FALSE(var.IsValid())
        << "Setting a value above of the valid value range.";

    var.SetDefaultValue(-5);
    ASSERT_FALSE(var.IsValid())
        << "Setting a value below of the valid value range.";

    var.SetDefaultValue(-1);
    ASSERT_EQ(-1, var.GetDefaultValue());

    var.SetDefaultValue(3);
    ASSERT_EQ(3, var.GetDefaultValue());

    ASSERT_EQ(MetaVariable::MetaVariableType_t::TYPE_S64, var.GetMetaType());
    ASSERT_TRUE(var.IsValid());
    ASSERT_EQ("S64", var.GetName());
    ASSERT_EQ(sizeof(int64_t), var.GetSize());

    //Make a copy (via stream) and compare
    std::stringstream ss;
    ASSERT_TRUE(var.Save(ss));

    MetaVariableInt<int64_t> copy;
    ASSERT_TRUE(copy.Load(ss));

    ASSERT_EQ(var.GetName(), copy.GetName());
    ASSERT_EQ(var.GetDefaultValue(), copy.GetDefaultValue());

    //Make a copy (via XML) and compare
    tinyxml2::XMLDocument doc;
    doc.Parse("<def></def>");

    auto root = doc.RootElement();
    ASSERT_TRUE(var.Save(doc, *root, "var"));

    ASSERT_TRUE(copy.Load(doc, *root->FirstChildElement()));

    ASSERT_EQ(var.GetName(), copy.GetName());
    ASSERT_EQ(var.GetDefaultValue(), copy.GetDefaultValue());
}

TEST(MetaVariableType, Int_u64)
{
    MetaVariableInt<uint64_t> var;
    var.SetName("U64");
    var.SetMinimumValue(1);
    var.SetMaximumValue(20);

    var.SetDefaultValue(0);

    ASSERT_FALSE(var.IsValid())
        << "Setting a value above of the valid value range.";

    var.SetDefaultValue(25);
    ASSERT_FALSE(var.IsValid())
        << "Setting a value below of the valid value range.";

    var.SetDefaultValue(15);
    ASSERT_EQ(15, var.GetDefaultValue());

    var.SetDefaultValue(3);
    ASSERT_EQ(3, var.GetDefaultValue());

    ASSERT_EQ(MetaVariable::MetaVariableType_t::TYPE_U64, var.GetMetaType());
    ASSERT_TRUE(var.IsValid());
    ASSERT_EQ("U64", var.GetName());
    ASSERT_EQ(sizeof(uint64_t), var.GetSize());

    //Make a copy (via stream) and compare
    std::stringstream ss;
    ASSERT_TRUE(var.Save(ss));

    MetaVariableInt<uint64_t> copy;
    ASSERT_TRUE(copy.Load(ss));

    ASSERT_EQ(var.GetName(), copy.GetName());
    ASSERT_EQ(var.GetDefaultValue(), copy.GetDefaultValue());

    //Make a copy (via XML) and compare
    tinyxml2::XMLDocument doc;
    doc.Parse("<def></def>");

    auto root = doc.RootElement();
    ASSERT_TRUE(var.Save(doc, *root, "var"));

    ASSERT_TRUE(copy.Load(doc, *root->FirstChildElement()));

    ASSERT_EQ(var.GetName(), copy.GetName());
    ASSERT_EQ(var.GetDefaultValue(), copy.GetDefaultValue());
}

TEST(MetaVariableType, Int_float)
{
    MetaVariableInt<float> var;
    var.SetName("FLOAT");
    var.SetMinimumValue(-10);
    var.SetMaximumValue(10);

    var.SetDefaultValue(12);

    ASSERT_FALSE(var.IsValid())
        << "Setting a value above of the valid value range.";

    var.SetDefaultValue(-12);
    ASSERT_FALSE(var.IsValid())
        << "Setting a value below of the valid value range.";

    var.SetDefaultValue(-3.14159f);
    ASSERT_EQ(-3.14159f, var.GetDefaultValue());

    var.SetDefaultValue(3.14159f);
    ASSERT_EQ(3.14159f, var.GetDefaultValue());

    ASSERT_EQ(MetaVariable::MetaVariableType_t::TYPE_FLOAT, var.GetMetaType());
    ASSERT_TRUE(var.IsValid());
    ASSERT_EQ("FLOAT", var.GetName());
    ASSERT_EQ(sizeof(float), var.GetSize());

    //Make a copy (via stream) and compare
    std::stringstream ss;
    ASSERT_TRUE(var.Save(ss));

    MetaVariableInt<float> copy;
    ASSERT_TRUE(copy.Load(ss));

    ASSERT_EQ(var.GetName(), copy.GetName());
    ASSERT_EQ(var.GetDefaultValue(), copy.GetDefaultValue());

    //Make a copy (via XML) and compare
    tinyxml2::XMLDocument doc;
    doc.Parse("<def></def>");

    auto root = doc.RootElement();
    ASSERT_TRUE(var.Save(doc, *root, "var"));

    ASSERT_TRUE(copy.Load(doc, *root->FirstChildElement()));

    ASSERT_EQ(var.GetName(), copy.GetName());
    ASSERT_EQ(var.GetDefaultValue(), copy.GetDefaultValue());
}

TEST(MetaVariableType, Int_double)
{
    MetaVariableInt<double> var;
    var.SetName("DOUBLE");
    var.SetMinimumValue(-10);
    var.SetMaximumValue(10);

    var.SetDefaultValue(12);

    ASSERT_FALSE(var.IsValid())
        << "Setting a value above of the valid value range.";

    var.SetDefaultValue(-12);
    ASSERT_FALSE(var.IsValid())
        << "Setting a value below of the valid value range.";

    var.SetDefaultValue(-3.14159);
    ASSERT_EQ(-3.14159, var.GetDefaultValue());

    var.SetDefaultValue(3.14159);
    ASSERT_EQ(3.14159, var.GetDefaultValue());

    ASSERT_EQ(MetaVariable::MetaVariableType_t::TYPE_DOUBLE, var.GetMetaType());
    ASSERT_TRUE(var.IsValid());
    ASSERT_EQ("DOUBLE", var.GetName());
    ASSERT_EQ(sizeof(double), var.GetSize());

    //Make a copy (via stream) and compare
    std::stringstream ss;
    ASSERT_TRUE(var.Save(ss));

    MetaVariableInt<double> copy;
    ASSERT_TRUE(copy.Load(ss));

    ASSERT_EQ(var.GetName(), copy.GetName());
    ASSERT_EQ(var.GetDefaultValue(), copy.GetDefaultValue());

    //Make a copy (via XML) and compare
    tinyxml2::XMLDocument doc;
    doc.Parse("<def></def>");

    auto root = doc.RootElement();
    ASSERT_TRUE(var.Save(doc, *root, "var"));

    ASSERT_TRUE(copy.Load(doc, *root->FirstChildElement()));

    ASSERT_EQ(var.GetName(), copy.GetName());
    ASSERT_EQ(var.GetDefaultValue(), copy.GetDefaultValue());
}

TEST(MetaVariableType, List)
{
    auto elementType = std::shared_ptr<MetaVariableInt<uint8_t>>(
        new MetaVariableInt<uint8_t>);

    MetaVariableList var(elementType);
    var.SetName("LIST");

    ASSERT_EQ(MetaVariable::MetaVariableType_t::TYPE_LIST, var.GetMetaType());
    ASSERT_EQ(MetaVariable::MetaVariableType_t::TYPE_U8,
        var.GetElementType()->GetMetaType());
    ASSERT_TRUE(var.IsValid());
    ASSERT_EQ("LIST", var.GetName());

    ASSERT_EQ(elementType, var.GetElementType());

    //Make a copy (via stream) and compare
    std::stringstream ss;
    ASSERT_TRUE(var.Save(ss));

    MetaVariableList copy(elementType);
    ASSERT_TRUE(copy.Load(ss));

    ASSERT_EQ(var.GetName(), copy.GetName());
    ASSERT_EQ(var.GetElementType()->GetMetaType(), copy.GetElementType()->GetMetaType());

    //Make a copy (via XML) and compare
    tinyxml2::XMLDocument doc;
    doc.Parse("<def></def>");

    auto root = doc.RootElement();
    ASSERT_TRUE(var.Save(doc, *root, "var"));

    ASSERT_TRUE(copy.Load(doc, *root->FirstChildElement()));

    ASSERT_EQ(var.GetName(), copy.GetName());
    ASSERT_EQ(var.GetElementType()->GetMetaType(), copy.GetElementType()->GetMetaType());
}

TEST(MetaVariableType, Map)
{
    auto keyType = std::shared_ptr<MetaVariableInt<uint8_t>>(
        new MetaVariableInt<uint8_t>);
    auto valueType = std::shared_ptr<MetaVariableInt<uint16_t>>(
        new MetaVariableInt<uint16_t>);

    MetaVariableMap var(keyType, valueType);
    var.SetName("MAP");

    ASSERT_EQ(MetaVariable::MetaVariableType_t::TYPE_MAP, var.GetMetaType());
    ASSERT_EQ(MetaVariable::MetaVariableType_t::TYPE_U8,
        var.GetKeyElementType()->GetMetaType());
    ASSERT_EQ(MetaVariable::MetaVariableType_t::TYPE_U16,
        var.GetValueElementType()->GetMetaType());
    ASSERT_TRUE(var.IsValid());
    ASSERT_EQ("MAP", var.GetName());

    ASSERT_EQ(keyType, var.GetKeyElementType());
    ASSERT_EQ(valueType, var.GetValueElementType());

    //Make a copy (via stream) and compare
    std::stringstream ss;
    ASSERT_TRUE(var.Save(ss));

    MetaVariableMap copy(keyType, valueType);
    ASSERT_TRUE(copy.Load(ss));

    ASSERT_EQ(var.GetName(), copy.GetName());
    ASSERT_EQ(var.GetKeyElementType()->GetMetaType(), copy.GetKeyElementType()->GetMetaType());

    //Make a copy (via XML) and compare
    tinyxml2::XMLDocument doc;
    doc.Parse("<def></def>");

    auto root = doc.RootElement();
    ASSERT_TRUE(var.Save(doc, *root, "var"));

    ASSERT_TRUE(copy.Load(doc, *root->FirstChildElement()));

    ASSERT_EQ(var.GetName(), copy.GetName());
    ASSERT_EQ(var.GetKeyElementType()->GetMetaType(), copy.GetKeyElementType()->GetMetaType());
}

TEST(MetaVariableType, Reference)
{
    MetaVariableReference var;
    var.SetName("REF");

    ASSERT_FALSE(var.IsValid())
        << "Checking validity with missing reference type.";

    ASSERT_FALSE(var.SetReferenceType("8InvalidReference"))
        << "Setting an invalidly named reference type.";

    ASSERT_TRUE(var.SetReferenceType("ValidReference"))
        << "Setting a validly named reference type.";

    var.SetPersistentParent(false);

    auto defaultedVar = std::shared_ptr<MetaVariable>(
        new MetaVariableInt<uint8_t>);
    std::dynamic_pointer_cast<MetaVariableInt<uint8_t>>(defaultedVar)->SetDefaultValue(5);

    var.AddDefaultedVariable(defaultedVar);

    ASSERT_EQ(MetaVariable::MetaVariableType_t::TYPE_REF, var.GetMetaType());
    ASSERT_TRUE(var.IsValid());
    ASSERT_EQ("REF", var.GetName());

    ASSERT_EQ(false, var.GetPersistentParent());
    ASSERT_EQ(1, var.GetDefaultedVariables().size());

    //Make a copy (via stream) and compare
    std::stringstream ss;
    ASSERT_TRUE(var.Save(ss));

    MetaVariableReference copy;
    ASSERT_TRUE(copy.Load(ss));

    ASSERT_EQ(var.GetName(), copy.GetName());
    ASSERT_EQ(var.GetPersistentParent(), copy.GetPersistentParent());
    ASSERT_EQ(var.GetDefaultedVariables().size(), copy.GetDefaultedVariables().size());

    //Make a copy (via XML) and compare
    tinyxml2::XMLDocument doc;
    doc.Parse("<def></def>");

    auto root = doc.RootElement();
    ASSERT_TRUE(var.Save(doc, *root, "var"));

    ASSERT_TRUE(copy.Load(doc, *root->FirstChildElement()));

    ASSERT_EQ(var.GetName(), copy.GetName());
    ASSERT_EQ(var.GetPersistentParent(), copy.GetPersistentParent());
    ASSERT_EQ(var.GetDefaultedVariables().size(), copy.GetDefaultedVariables().size());
}

TEST(MetaVariableType, String)
{
    MetaVariableString var;
    var.SetName("STRING");
    var.SetSize(10);
    var.SetEncoding(MetaVariableString::Encoding_t::ENCODING_UTF8);

    ASSERT_EQ(MetaVariable::MetaVariableType_t::TYPE_STRING, var.GetMetaType());
    ASSERT_TRUE(var.IsValid());
    ASSERT_EQ("STRING", var.GetName());

    ASSERT_EQ(10, var.GetSize());
    ASSERT_EQ(MetaVariableString::Encoding_t::ENCODING_UTF8, var.GetEncoding());

    //No numbers
    var.SetRegularExpression("/^([^0-9]*)$/");
    ASSERT_EQ("/^([^0-9]*)$/", var.GetRegularExpression());

    var.SetDefaultValue("1 string");
    ASSERT_FALSE(var.IsValid())
        << "Checking validity with a value disallowed by the regex.";

    var.SetRegularExpression("");
    ASSERT_TRUE(var.IsValid())
        << "Checking validity after removing the regex.";

    var.SetDefaultValue("A string");
    ASSERT_EQ("A string", var.GetDefaultValue());

    //Make a copy (via stream) and compare
    std::stringstream ss;
    ASSERT_TRUE(var.Save(ss));

    MetaVariableString copy;
    ASSERT_TRUE(copy.Load(ss));

    ASSERT_EQ(var.GetName(), copy.GetName());
    ASSERT_EQ(var.GetSize(), copy.GetSize());
    ASSERT_EQ(var.GetEncoding(), copy.GetEncoding());
    ASSERT_EQ(var.GetRegularExpression(), copy.GetRegularExpression());
    ASSERT_EQ(var.GetDefaultValue(), copy.GetDefaultValue());

    //Make a copy (via XML) and compare
    tinyxml2::XMLDocument doc;
    doc.Parse("<def></def>");

    auto root = doc.RootElement();
    ASSERT_TRUE(var.Save(doc, *root, "var"));

    ASSERT_TRUE(copy.Load(doc, *root->FirstChildElement()));

    ASSERT_EQ(var.GetName(), copy.GetName());
    ASSERT_EQ(var.GetSize(), copy.GetSize());
    ASSERT_EQ(var.GetEncoding(), copy.GetEncoding());
    ASSERT_EQ(var.GetRegularExpression(), copy.GetRegularExpression());
    ASSERT_EQ(var.GetDefaultValue(), copy.GetDefaultValue());
}

TEST(MetaVariableInt, StringToValue)
{
    bool ok = false;

    double val = libobjgen::MetaVariableInt<double>::StringToValue(
        "-3.14159e10", &ok);

    ASSERT_TRUE(ok);
    ASSERT_EQ(val, -3.14159e10);
}

int main(int argc, char *argv[])
{
    try
    {
        ::testing::InitGoogleTest(&argc, argv);

        return RUN_ALL_TESTS();
    }
    catch(...)
    {
        return EXIT_FAILURE;
    }
}
