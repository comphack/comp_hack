/**
 * @file libobjgen/tests/MetaObject.cpp
 * @ingroup libobjgen
 *
 * @author HACKfrost
 *
 * @brief Test the MetaObject class.
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
#include <MetaObject.h>
#include <MetaObjectXmlParser.h>
#include <MetaVariable.h>

using namespace libobjgen;

TEST(MetaObject, Validate)
{
    MetaObject obj;

    ASSERT_FALSE(obj.IsValid())
        << "Attempting to validate an object with nothing set on it.";

    ASSERT_FALSE(obj.SetName("2Test"))
        << "Attempting to set an invalid name.";

    ASSERT_TRUE(obj.SetName("Test"))
        << "Attempting to set a valid name.";

    ASSERT_FALSE(obj.IsValid())
        << "Attempting to validate an object with no variables or base object.";

    ASSERT_FALSE(obj.SetBaseObject("2TestBase"))
        << "Attempting to set an invalid base object name.";

    ASSERT_TRUE(obj.SetBaseObject("TestBase"))
        << "Attempting to set a valid base object name.";

    ASSERT_TRUE(obj.SetBaseObject("test::TestBase"))
        << "Attempting to set a valid base object name with a namespace.";

    ASSERT_FALSE(obj.SetNamespace("2test"))
        << "Attempting to set an invalid namespace.";

    ASSERT_TRUE(obj.SetNamespace("test2"))
        << "Attempting to set a valid namespace.";

    ASSERT_TRUE(obj.IsValid())
        << "Attempting to validate a derived object with no variables.";

    //Clear the base object
    obj.SetBaseObject("");

    auto var = MetaVariable::CreateType("bool");
    var->SetName("Boolean");
    ASSERT_TRUE(obj.AddVariable(var));

    ASSERT_TRUE(obj.IsValid());

    obj.SetSourceLocation("somedb");

    ASSERT_FALSE(obj.IsValid())
        << "Attempting to validate a non-persistent object with a source location.";

    obj.SetPersistent(true);

    ASSERT_TRUE(obj.IsValid());
}

TEST(MetaObject, StreamCopy)
{
    MetaObject obj;
    obj.SetName("Test");
    obj.SetBaseObject("TestBase");

    //Add a couple members that don't require much validation
    auto var = MetaVariable::CreateType("bool");
    var->SetName("Boolean");
    ASSERT_TRUE(obj.AddVariable(var));

    var = MetaVariable::CreateType("u8");
    var->SetName("Unsigned8");
    ASSERT_TRUE(obj.AddVariable(var));

    var = MetaVariable::CreateType("s16");
    var->SetName("Signed16");
    ASSERT_TRUE(obj.AddVariable(var));
    ASSERT_FALSE(obj.AddVariable(var))
        << "Attempting to add the samve variable twice.";

    var = MetaVariable::CreateType("s16");
    var->SetName("SecondSigned16");
    ASSERT_TRUE(obj.AddVariable(var));
    ASSERT_TRUE(obj.RemoveVariable("SecondSigned16"));

    MetaObject copy;

    std::stringstream ss;
    ASSERT_TRUE(obj.Save(ss));
    ASSERT_TRUE(copy.Load(ss));

    ASSERT_EQ(copy.GetName(), obj.GetName());
    ASSERT_EQ(copy.GetBaseObject(), obj.GetBaseObject());
    ASSERT_EQ(copy.IsPersistent(), obj.IsPersistent());
    ASSERT_EQ(copy.IsScriptEnabled(), obj.IsScriptEnabled());

    int varCount = 0;
    auto objIter = obj.VariablesBegin();
    auto copyIter = copy.VariablesBegin();
    while(objIter != obj.VariablesEnd())
    {
        ASSERT_TRUE(copyIter != copy.VariablesEnd());
        if(copyIter == copy.VariablesEnd())
        {
            break;
        }

        auto objVar = *objIter;
        auto copyVar = *copyIter;

        ASSERT_EQ(copyVar->GetName(), objVar->GetName());
        ASSERT_EQ(copyVar->GetMetaType(), objVar->GetMetaType());

        objIter++;
        copyIter++;
        varCount++;
    }

    ASSERT_EQ(3, varCount);
}

TEST(MetaObjectXmlParser, XmlCopy)
{
    MetaObject obj;
    obj.SetName("Test");

    auto var = MetaVariable::CreateType("bool");
    var->SetName("Boolean");
    ASSERT_TRUE(obj.AddVariable(var));

    tinyxml2::XMLDocument doc;
    doc.Parse("<def></def>");

    auto root = doc.RootElement();
    ASSERT_TRUE(obj.Save(doc, *root));

    MetaObjectXmlParser parser;

    ASSERT_TRUE(parser.Load(doc, *root->FirstChildElement()));

    auto copy = parser.GetCurrentObject();
    ASSERT_EQ(copy->GetName(), obj.GetName());
    ASSERT_EQ(copy->GetBaseObject(), obj.GetBaseObject());
    ASSERT_EQ(copy->IsPersistent(), obj.IsPersistent());
    ASSERT_EQ(copy->IsScriptEnabled(), obj.IsScriptEnabled());

    int varCount = 0;
    auto objIter = obj.VariablesBegin();
    auto copyIter = copy->VariablesBegin();
    while (objIter != obj.VariablesEnd())
    {
        ASSERT_TRUE(copyIter != copy->VariablesEnd());
        if (copyIter == copy->VariablesEnd())
        {
            break;
        }

        auto objVar = *objIter;
        auto copyVar = *copyIter;

        ASSERT_EQ(copyVar->GetName(), objVar->GetName());
        ASSERT_EQ(copyVar->GetMetaType(), objVar->GetMetaType());

        objIter++;
        copyIter++;
        varCount++;
    }

    ASSERT_EQ(1, varCount);
}

TEST(MetaObjectXmlParser, ValidReferenceCheck)
{
    auto xml = 
        "<objects>"
            "<object name='Object1' persistent='false'>"
                "<member name='Unsigned8' type='u8'/>"
            "</object>"
            "<object name='Object2' persistent='false'>"
                "<member name='Object1' type='Object1*'/>"
            "</object>"
        "</objects>";

    MetaObjectXmlParser parser;

    tinyxml2::XMLDocument doc;
    doc.Parse(xml);

    const tinyxml2::XMLElement *pObjectXml = doc.RootElement()->FirstChildElement("object");

    while(nullptr != pObjectXml)
    {
        ASSERT_TRUE(parser.LoadTypeInformation(doc, *pObjectXml));
        pObjectXml = pObjectXml->NextSiblingElement("object");
    }

    ASSERT_TRUE(parser.FinalizeObjectAndReferences("Object1"));
}

TEST(MetaObjectXmlParser, CircularReferenceCheck)
{
    auto xml = 
        "<objects>"
            "<object name='Object1' persistent='false'>"
                "<member name='Object2' type='Object2*'/>"
            "</object>"
            "<object name='Object2' persistent='false'>"
                "<member name='Object1' type='Object1*'/>"
            "</object>"
        "</objects>";

    MetaObjectXmlParser parser;

    tinyxml2::XMLDocument doc;
    doc.Parse(xml);

    const tinyxml2::XMLElement *pObjectXml = doc.RootElement()->FirstChildElement("object");

    while(nullptr != pObjectXml)
    {
        ASSERT_TRUE(parser.LoadTypeInformation(doc, *pObjectXml));
        pObjectXml = pObjectXml->NextSiblingElement("object");
    }

    ASSERT_FALSE(parser.FinalizeObjectAndReferences("Object1"));

    //Reset and make them persistent
    parser = MetaObjectXmlParser();

    pObjectXml = doc.RootElement()->FirstChildElement("object");

    while(nullptr != pObjectXml)
    {
        ASSERT_TRUE(parser.LoadTypeInformation(doc, *pObjectXml));
        pObjectXml = pObjectXml->NextSiblingElement("object");
    }

    auto obj = parser.GetKnownObject("Object1");
    obj->SetPersistent(true);

    obj = parser.GetKnownObject("Object2");
    obj->SetPersistent(true);

    ASSERT_TRUE(parser.FinalizeObjectAndReferences("Object1"));
}

TEST(MetaObjectXmlParser, ScriptEnabledCheck)
{
    auto xml = 
        "<objects>"
            "<object name='Object1' scriptenabled='false'>"
                "<member name='Field1' type='u8'/>"
            "</object>"
            "<object name='Object2' baseobject='Object1' scriptenabled='true'>"
                "<member name='Object3' type='Object3*'/>"
            "</object>"
            "<object name='Object3' scriptenabled='false'>"
                "<member name='Field2' type='u8'/>"
            "</object>"
        "</objects>";

    MetaObjectXmlParser parser;

    tinyxml2::XMLDocument doc;
    doc.Parse(xml);

    const tinyxml2::XMLElement *pObjectXml = doc.RootElement()->FirstChildElement("object");

    while(nullptr != pObjectXml)
    {
        ASSERT_TRUE(parser.LoadTypeInformation(doc, *pObjectXml));
        pObjectXml = pObjectXml->NextSiblingElement("object");
    }

    //Building Object1 or Object2 is not a problem until we try to build the derived object too
    ASSERT_TRUE(parser.FinalizeObjectAndReferences("Object1"));
    ASSERT_TRUE(parser.FinalizeObjectAndReferences("Object3"));
    ASSERT_FALSE(parser.FinalizeObjectAndReferences("Object2"));

    //Reset and make the base object script enabled
    parser = MetaObjectXmlParser();

    pObjectXml = doc.RootElement()->FirstChildElement("object");

    while(nullptr != pObjectXml)
    {
        ASSERT_TRUE(parser.LoadTypeInformation(doc, *pObjectXml));
        pObjectXml = pObjectXml->NextSiblingElement("object");
    }

    auto obj = parser.GetKnownObject("Object1");
    obj->SetScriptEnabled(true);

    ASSERT_FALSE(parser.FinalizeObjectAndReferences("Object2"));

    //Reset and make the referenced object script enabled as well
    parser = MetaObjectXmlParser();

    pObjectXml = doc.RootElement()->FirstChildElement("object");

    while (nullptr != pObjectXml)
    {
        ASSERT_TRUE(parser.LoadTypeInformation(doc, *pObjectXml));
        pObjectXml = pObjectXml->NextSiblingElement("object");
    }

    obj = parser.GetKnownObject("Object3");
    obj->SetScriptEnabled(true);

    ASSERT_FALSE(parser.FinalizeObjectAndReferences("Object2"));
}

TEST(MetaObjectXmlParser, ParseAllObjectAttributes)
{
    auto xml = 
        "<objects>"
            "<object name='Object1' namespace='ns1' persistent='true'"
                    " location='test' scriptenabled='true'>"
                "<member name='Unsigned8' type='u8'/>"
            "</object>"
            "<object name='Object2' baseobject='ns1::Object1' persistent='false'/>"
            "<object name='Object3' namespace='ns2' baseobject='Object2' persistent='false'/>"
        "</objects>";

    MetaObjectXmlParser parser;

    tinyxml2::XMLDocument doc;
    doc.Parse(xml);

    const tinyxml2::XMLElement *pObjectXml = doc.RootElement()->FirstChildElement("object");

    while(nullptr != pObjectXml)
    {
        ASSERT_TRUE(parser.LoadTypeInformation(doc, *pObjectXml));
        pObjectXml = pObjectXml->NextSiblingElement("object");
    }

    ASSERT_TRUE(parser.FinalizeObjectAndReferences("Object1"));

    auto obj1 = parser.GetKnownObject("Object1");
    auto obj2 = parser.GetKnownObject("Object2");
    auto obj3 = parser.GetKnownObject("Object3");

    ASSERT_EQ(obj1->GetNamespace(), "ns1");
    ASSERT_EQ(obj2->GetNamespace(), "objects");
    ASSERT_EQ(obj3->GetNamespace(), "ns2");

    ASSERT_EQ(obj1->IsPersistent(), true);
    ASSERT_EQ(obj2->IsPersistent(), false);
    ASSERT_EQ(obj3->IsPersistent(), false);

    ASSERT_EQ(obj1->GetSourceLocation(), "test");
    ASSERT_EQ(obj2->GetSourceLocation(), "");
    ASSERT_EQ(obj3->GetSourceLocation(), "");

    ASSERT_EQ(obj1->IsScriptEnabled(), true);
    ASSERT_EQ(obj2->IsScriptEnabled(), false);
    ASSERT_EQ(obj3->IsScriptEnabled(), false);

    ASSERT_EQ(obj1->GetBaseObject(), "");
    ASSERT_EQ(obj2->GetBaseObject(), "ns1::Object1");
    ASSERT_EQ(obj3->GetBaseObject(), "objects::Object2");
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
