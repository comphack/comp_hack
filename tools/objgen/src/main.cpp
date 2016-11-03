/**
 * @file tools/objgen/src/main.cpp
 * @ingroup objgen
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Utility to generate C++ objects from XML data structures
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

#include <cstdlib>

// Standard C++11 Includes
#include <iostream>
#include <fstream>
#include <streambuf>

// tinyxml2 Includes
#include "PushIgnore.h"
#include <tinyxml2.h>
#include "PopIgnore.h"

// libobjgen Includes
#include "Generator.h"
#include "GeneratorFactory.h"
#include "MetaObject.h"
#include "MetaVariable.h"
#include "MetaVariableInt.h"
#include "MetaVariableString.h"

std::unordered_map<std::string, std::shared_ptr<
    libobjgen::MetaObject>> gObjects;

bool LoadObjects(const std::list<std::string>& searchPath,
    const std::string& xmlFile)
{
    tinyxml2::XMLDocument doc;

    bool loaded = tinyxml2::XML_NO_ERROR == doc.LoadFile(xmlFile.c_str());

    if(!loaded)
    {
        for(auto path : searchPath)
        {
            std::string nextPath = path + std::string("/") + xmlFile;

            loaded = tinyxml2::XML_NO_ERROR == doc.LoadFile(nextPath.c_str());

            if(loaded)
            {
                break;
            }
        }
    }

    if(!loaded)
    {
        std::cerr << "Failed to parse XML file: " << xmlFile << std::endl;
        std::cerr << "Check the path and the file contents." << std::endl;

        return false;
    }

    tinyxml2::XMLElement *pRoot = doc.RootElement();

    if(nullptr == pRoot)
    {
        std::cerr << "Invalid object XML format for file: "
            << xmlFile << std::endl;

        return false;
    }

    if(nullptr == pRoot->Name() || "objgen" != std::string(pRoot->Name()))
    {
        std::cerr << "Invalid root element in object XML format for file: "
            << xmlFile << std::endl;

        return false;
    }

    const tinyxml2::XMLElement *pIncludeXml =
        pRoot->FirstChildElement("include");

    while(nullptr != pIncludeXml)
    {
        const char *szPath = pIncludeXml->Attribute("path");

        if(nullptr == szPath)
        {
            std::cerr << "Missing path attribute in include element in "
                "object XML format for file: " << xmlFile << std::endl;

            return false;
        }

        std::string includePath = szPath;

        if(!LoadObjects(searchPath, includePath))
        {
            return false;
        }

        pIncludeXml = pIncludeXml->NextSiblingElement("include");
    }

    const tinyxml2::XMLElement *pObjectXml = pRoot->FirstChildElement("object");

    while(nullptr != pObjectXml)
    {
        std::shared_ptr<libobjgen::MetaObject> obj(new libobjgen::MetaObject);

        if(!obj->Load(doc, *pObjectXml))
        {
            std::cerr << "Failed to read object: " << obj->GetName()
                << ":  " << obj->GetError() << std::endl;

            return false;
        }

        gObjects[obj->GetName()] = obj;

        pObjectXml = pObjectXml->NextSiblingElement("object");
    }

    return true;
}

bool GenerateFile(const std::string& path, const std::string& extension,
    const std::string& object)
{
    auto objectPair = gObjects.find(object);

    if(gObjects.end() == objectPair)
    {
        std::cerr << "Failed to find object '" << object
            << "' for output file: " << path << std::endl;

        return false;
    }

    std::shared_ptr<libobjgen::Generator> generator =
        libobjgen::GeneratorFactory().Generator(extension);

    if(!generator)
    {
        std::cerr << "Unknown file extension: " << extension << std::endl;

        return false;
    }

    std::string code = generator->Generate(*objectPair->second.get());

    if(code.empty())
    {
        std::cerr << "Failed to generate code for object: "
            << object << std::endl;

        return false;
    }

    bool rewrite = true;

    {
        std::ifstream inFile;

        inFile.open(path.c_str());

        if(inFile.good())
        {
            rewrite = code != std::string(
                std::istreambuf_iterator<char>(inFile),
                std::istreambuf_iterator<char>());
        }
    }

    if(rewrite)
    {
        std::ofstream outFile;

        outFile.open(path.c_str());

        if(!outFile.good())
        {
            std::cerr << "" << std::endl;

            return false;
        }

        outFile << code;

        if(!outFile.good())
        {
            std::cerr << "" << std::endl;

            return false;
        }
    }

    return true;
}

int main(int argc, char *argv[])
{
    typedef enum
    {
        LAST_OPTION_NONE = 0,
        LAST_OPTION_INCLUDE,
        LAST_OPTION_OUTPUT,
    } LastOption_t;

    LastOption_t lastOption = LAST_OPTION_NONE;

    std::list<std::string> searchPath;
    std::list<std::string> xmlFiles;
    std::list<std::string> outputFiles;

    for(int i = 1; i < argc; ++i)
    {
        std::string opt = argv[i];

        switch(lastOption)
        {
            case LAST_OPTION_INCLUDE:
            {
                searchPath.push_back(opt);

                lastOption = LAST_OPTION_NONE;
                break;
            }
            case LAST_OPTION_OUTPUT:
            {
                outputFiles.push_back(opt);

                lastOption = LAST_OPTION_NONE;
                break;
            }
            default:
            {
                std::smatch match;

                if(std::regex_match(opt, match, std::regex("^[-]I(.*)$")))
                {
                    std::string include = match[1];

                    lastOption = include.empty() ? LAST_OPTION_INCLUDE :
                        LAST_OPTION_NONE;

                    if(!include.empty())
                    {
                        searchPath.push_back(include);
                    }
                }
                else if("-o" == opt)
                {
                    lastOption = LAST_OPTION_OUTPUT;
                }
                else
                {
                    xmlFiles.push_back(opt);
                }
                break;
            }
        }
    }

    if(LAST_OPTION_NONE != lastOption)
    {
        std::cerr << "Argument expected after: " << argv[argc - 1] << std::endl;

        return EXIT_FAILURE;
    }

    for(auto xmlFile : xmlFiles)
    {
        if(!LoadObjects(searchPath, xmlFile))
        {
            return EXIT_FAILURE;
        }
    }

    for(auto outputFile : outputFiles)
    {
        std::smatch match;
        std::string path;
        std::string extension;
        std::string object;

        if(std::regex_match(outputFile, match, std::regex(
            "^(.*\\/)?([^\\/]+)[.]([^.=]+)(?:[=](.+))?$")))
        {
            object = match[4];

            if(object.empty())
            {
                object = match[2];
            }

            extension = std::string(match[3]);
            path = std::string(match[1]) + std::string(match[2]) +
                std::string(".") + extension;

            std::transform(extension.begin(), extension.end(),
                extension.begin(), ::tolower);

            if(!GenerateFile(path, extension, object))
            {
                return EXIT_FAILURE;
            }
        }
        else
        {
            std::cerr << "Invalid output file name: "
                << outputFile << std::endl;

            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}
