/**
 * @file libcomp/src/DatabaseQuery.h
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Base class to handle a database query.
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

#ifndef LIBCOMP_SRC_DATABASEQUERY_H
#define LIBCOMP_SRC_DATABASEQUERY_H

// libobjgen Includes
#include "UUID.h"

// libcomp Includes
#include "CString.h"

// Standard C++11 Includes
#include <unordered_map>

namespace libcomp
{

class DatabaseQueryImpl
{
public:
    virtual ~DatabaseQueryImpl();

    virtual bool Prepare(const String& query) = 0;
    virtual bool Execute() = 0;
    virtual bool Next() = 0;

    virtual bool Bind(size_t index, const String& value) = 0;
    virtual bool Bind(const String& name, const String& value) = 0;
    virtual bool Bind(size_t index, const std::vector<char>& value) = 0;
    virtual bool Bind(const String& name, const std::vector<char>& value) = 0;
    virtual bool Bind(size_t index, const libobjgen::UUID& value) = 0;
    virtual bool Bind(const String& name, const libobjgen::UUID& value) = 0;
    virtual bool Bind(size_t index, int32_t value) = 0;
    virtual bool Bind(const String& name, int32_t value) = 0;
    virtual bool Bind(size_t index, int64_t value) = 0;
    virtual bool Bind(const String& name, int64_t value) = 0;
    virtual bool Bind(size_t index, float value) = 0;
    virtual bool Bind(const String& name, float value) = 0;
    virtual bool Bind(size_t index, double value) = 0;
    virtual bool Bind(const String& name, double value) = 0;
    virtual bool Bind(size_t index, bool value) = 0;
    virtual bool Bind(const String& name, bool value) = 0;
    virtual bool Bind(size_t index, const std::unordered_map<
        std::string, std::vector<char>>& values);
    virtual bool Bind(const String& name, const std::unordered_map<
        std::string, std::vector<char>>& values);

    virtual bool GetValue(size_t index, String& value) = 0;
    virtual bool GetValue(const String& name, String& value) = 0;
    virtual bool GetValue(size_t index, std::vector<char>& value) = 0;
    virtual bool GetValue(const String& name, std::vector<char>& value) = 0;
    virtual bool GetValue(size_t index, libobjgen::UUID& value) = 0;
    virtual bool GetValue(const String& name, libobjgen::UUID& value) = 0;
    virtual bool GetValue(size_t index, int32_t& value) = 0;
    virtual bool GetValue(const String& name, int32_t& value) = 0;
    virtual bool GetValue(size_t index, int64_t& value) = 0;
    virtual bool GetValue(const String& name, int64_t& value) = 0;
    virtual bool GetValue(size_t index, float& value) = 0;
    virtual bool GetValue(const String& name, float& value) = 0;
    virtual bool GetValue(size_t index, double& value) = 0;
    virtual bool GetValue(const String& name, double& value) = 0;
    virtual bool GetValue(size_t index, bool& value) = 0;
    virtual bool GetValue(const String& name, bool& value) = 0;
    virtual bool GetMap(size_t index, std::unordered_map<
        std::string, std::vector<char>>& values);
    virtual bool GetMap(const String& name, std::unordered_map<
        std::string, std::vector<char>>& values);
    virtual bool GetRows(std::list<std::unordered_map<
        std::string, std::vector<char>>>& rows);

    virtual bool BatchNext() = 0;

    virtual bool IsValid() const = 0;
};

class DatabaseQuery
{
public:
    DatabaseQuery(DatabaseQueryImpl *pImpl);
    DatabaseQuery(DatabaseQueryImpl *pImpl, const String& query);
    DatabaseQuery(const DatabaseQuery& other) = delete;
    DatabaseQuery(DatabaseQuery&& other);
    ~DatabaseQuery();

    bool Prepare(const String& query);
    bool Execute();
    bool Next();

    bool Bind(size_t index, const String& value);
    bool Bind(const String& name, const String& value);
    bool Bind(size_t index, const std::vector<char>& value);
    bool Bind(const String& name, const std::vector<char>& value);
    bool Bind(size_t index, const libobjgen::UUID& value);
    bool Bind(const String& name, const libobjgen::UUID& value);
    bool Bind(size_t index, int32_t value);
    bool Bind(const String& name, int32_t value);
    bool Bind(size_t index, int64_t value);
    bool Bind(const String& name, int64_t value);
    bool Bind(size_t index, float value);
    bool Bind(const String& name, float value);
    bool Bind(size_t index, double value);
    bool Bind(const String& name, double value);
    bool Bind(size_t index, bool value);
    bool Bind(const String& name, bool value);
    bool Bind(size_t index, const std::unordered_map<std::string,
        std::vector<char>>& values);
    bool Bind(const String& name, const std::unordered_map<
        std::string, std::vector<char>>& values);

    bool GetValue(size_t index, String& value);
    bool GetValue(const String& name, String& value);
    bool GetValue(size_t index, std::vector<char>& value);
    bool GetValue(const String& name, std::vector<char>& value);
    bool GetValue(size_t index, libobjgen::UUID& value);
    bool GetValue(const String& name, libobjgen::UUID& value);
    bool GetValue(size_t index, int32_t& value);
    bool GetValue(const String& name, int32_t& value);
    bool GetValue(size_t index, int64_t& value);
    bool GetValue(const String& name, int64_t& value);
    bool GetValue(size_t index, float& value);
    bool GetValue(const String& name, float& value);
    bool GetValue(size_t index, double& value);
    bool GetValue(const String& name, double& value);
    bool GetValue(size_t index, bool& value);
    bool GetValue(const String& name, bool& value);
    bool GetMap(size_t index, std::unordered_map<
        std::string, std::vector<char>>& values);
    bool GetMap(const String& name, std::unordered_map<
        std::string, std::vector<char>>& values);
    virtual bool GetRows(std::list<std::unordered_map<
        std::string, std::vector<char>>>& rows);

    bool BatchNext();

    bool IsValid() const;

    DatabaseQuery& operator=(DatabaseQuery&& other);

protected:
    DatabaseQueryImpl *mImpl;
};

} // namespace libcomp

#endif // LIBCOMP_SRC_DATABASEQUERY_H
