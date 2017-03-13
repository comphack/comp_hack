/**
 * @file libcomp/src/DatabaseQuerySQLite3.cpp
 * @ingroup libcomp
 *
 * @author HACKfrost
 *
 * @brief A SQLite3 database query.
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

#include "DatabaseQuerySQLite3.h"
#include "DatabaseSQLite3.h"

 // SQLite3 Includes
#include <sqlite3.h>

// Standard C++11 Includes
#include <thread>
#include <chrono>

using namespace libcomp;

DatabaseQuerySQLite3::DatabaseQuerySQLite3(sqlite3 *pDatabase,
    uint8_t maxRetryCount, uint16_t retryDelay)
    : mDatabase(pDatabase), mStatement(nullptr), mStatus(SQLITE_OK),
    mDidJustExecute(false), mMaxRetryCount(maxRetryCount),
    mRetryDelay(retryDelay)
{
}

DatabaseQuerySQLite3::~DatabaseQuerySQLite3()
{
    if(nullptr != mStatement)
    {
        sqlite3_finalize(mStatement);
    }
}

bool DatabaseQuerySQLite3::Prepare(const String& query)
{
    int len = (int)query.Length();
    mStatus = sqlite3_prepare_v2(mDatabase, query.C(),
        len, &mStatement, nullptr);

    return IsValid();
}

bool DatabaseQuerySQLite3::Execute()
{
    if(!IsValid())
    {
        return false;
    }

    // To circumvent the limitations of SQLite multi-process access,
    // allow a configurable retry count and a delay
    uint8_t attempts = 0;
    do
    {
        if(attempts++ > 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(mRetryDelay));
        }

        mStatus = sqlite3_step(mStatement);
    } while(mStatus == SQLITE_BUSY && attempts < mMaxRetryCount);

    mDidJustExecute = true;
    
    int colCount = sqlite3_column_count(mStatement);

    if(colCount > 0)
    {
        for(int i = 0; i < colCount; i++)
        {
            mResultColumnNames.push_back(std::string(sqlite3_column_name(mStatement, i)));
            mResultColumnTypes.push_back(sqlite3_column_type(mStatement, i));
        }
    }

    return IsValid();
}

bool DatabaseQuerySQLite3::Next()
{
    if(!mDidJustExecute)
    {
        mStatus = sqlite3_step(mStatement);
    }
    else
    {
        mDidJustExecute = false;
        return SQLITE_DONE != mStatus;
    }

    return IsValid() && SQLITE_DONE != mStatus;
}

bool DatabaseQuerySQLite3::Bind(size_t index, const String& value)
{
    int idx = (int)index;
    int len = (int)value.Size();
    mStatus = sqlite3_bind_text(mStatement, idx,
        value.C(), len, SQLITE_TRANSIENT);
    return IsValid();
}

bool DatabaseQuerySQLite3::Bind(const String& name, const String& value)
{
    size_t index = GetNamedBindingIndex(name);

    if(index == 0)
    {
        mStatus = SQLITE_ERROR;
    }

    return IsValid() && Bind(index, value);
}

bool DatabaseQuerySQLite3::Bind(size_t index, const std::vector<char>& value)
{
    if(nullptr != mStatement)
    {
        int idx = (int)index;
        int size = (int)value.size();
        mStatus = sqlite3_bind_blob(mStatement, idx, &value[0], size, SQLITE_TRANSIENT);
    }
    else
    {
        return false;
    }

    return IsValid();
}

bool DatabaseQuerySQLite3::Bind(const String& name,
    const std::vector<char>& value)
{
    size_t index = GetNamedBindingIndex(name);

    if(index == 0)
    {
        mStatus = SQLITE_ERROR;
    }

    return IsValid() && Bind(index, value);
}

bool DatabaseQuerySQLite3::Bind(size_t index, const libobjgen::UUID& value)
{
    auto uuidStr = libcomp::String(value.ToString());
    return Bind(index, uuidStr);
}

bool DatabaseQuerySQLite3::Bind(const String& name,
    const libobjgen::UUID& value)
{
    auto uuidStr = libcomp::String(value.ToString());
    return Bind(name, uuidStr);
}

bool DatabaseQuerySQLite3::Bind(size_t index, int32_t value)
{
    int idx = (int)index;
    mStatus = sqlite3_bind_int(mStatement, idx, value);
    return IsValid();
}

bool DatabaseQuerySQLite3::Bind(const String& name, int32_t value)
{
    size_t index = GetNamedBindingIndex(name);

    if(index == 0)
    {
        mStatus = SQLITE_ERROR;
    }

    return IsValid() && Bind(index, value);
}

bool DatabaseQuerySQLite3::Bind(size_t index, int64_t value)
{
    int idx = (int)index;
    mStatus = sqlite3_bind_int64(mStatement, idx, value);
    return IsValid();
}

bool DatabaseQuerySQLite3::Bind(const String& name, int64_t value)
{
    size_t index = GetNamedBindingIndex(name);

    if(index == 0)
    {
        mStatus = SQLITE_ERROR;
    }

    return IsValid() && Bind(index, value);
}

bool DatabaseQuerySQLite3::Bind(size_t index, float value)
{
    int idx = (int)index;
    double doubleValue = (double)value;
    mStatus = sqlite3_bind_double(mStatement, idx, doubleValue);
    return IsValid();
}

bool DatabaseQuerySQLite3::Bind(const String& name, float value)
{
    size_t index = GetNamedBindingIndex(name);

    if(index == 0)
    {
        mStatus = SQLITE_ERROR;
    }

    return IsValid() && Bind(index, value);
}

bool DatabaseQuerySQLite3::Bind(size_t index, double value)
{
    int idx = (int)index;
    mStatus = sqlite3_bind_double(mStatement, idx, value);
    return IsValid();
}

bool DatabaseQuerySQLite3::Bind(const String& name, double value)
{
    size_t index = GetNamedBindingIndex(name);

    if(index == 0)
    {
        mStatus = SQLITE_ERROR;
    }

    return IsValid() && Bind(index, value);
}

bool DatabaseQuerySQLite3::Bind(size_t index, bool value)
{
    int idx = (int)index;
    int boolValue = (int)value;
    mStatus = sqlite3_bind_int(mStatement, idx, boolValue);
    return IsValid();
}

bool DatabaseQuerySQLite3::Bind(const String& name, bool value)
{
    size_t index = GetNamedBindingIndex(name);

    if(index == 0)
    {
        mStatus = SQLITE_ERROR;
    }

    return IsValid() && Bind(index, value);
}

bool DatabaseQuerySQLite3::Bind(size_t index, const std::unordered_map<
    std::string, std::vector<char>>& values)
{
    (void)index;
    (void)values;

    /// @todo
    return false;
}

bool DatabaseQuerySQLite3::Bind(const String& name, const std::unordered_map<
    std::string, std::vector<char>>& values)
{
    (void)name;
    (void)values;

    /// @todo
    return false;
}

bool DatabaseQuerySQLite3::GetValue(size_t index, String& value)
{
    if(mResultColumnTypes.size() <= index
        || mResultColumnTypes[index] != SQLITE_TEXT)
    {
        return false;
    }

    int idx = (int)index;

    size_t bytes = (size_t)sqlite3_column_bytes(mStatement, idx);

    auto val = sqlite3_column_text(mStatement, idx);

    std::string str(val, val + bytes);
    value = libcomp::String(str);

    return true;
}

bool DatabaseQuerySQLite3::GetValue(const String& name, String& value)
{
    size_t index;
    if(!GetResultColumnIndex(name, index))
    {
        return false;
    }

    return GetValue(index, value);
}

bool DatabaseQuerySQLite3::GetValue(size_t index, std::vector<char>& value)
{
    if(mResultColumnTypes.size() <= index
        || mResultColumnTypes[index] != SQLITE_BLOB)
    {
        return false;
    }

    int idx = (int)index;

    size_t bytes = (size_t)sqlite3_column_bytes(mStatement, idx);

    auto val = (const char*)sqlite3_column_blob(mStatement, idx);

    value.clear();
    value.insert(value.begin(), val, val + bytes);

    return true;
}

bool DatabaseQuerySQLite3::GetValue(const String& name,
    std::vector<char>& value)
{
    size_t index;
    if(!GetResultColumnIndex(name, index))
    {
        return false;
    }

    return GetValue(index, value);
}

bool DatabaseQuerySQLite3::GetValue(size_t index, libobjgen::UUID& value)
{
    libcomp::String uuidStr;
    if(GetValue(index, uuidStr))
    {
        value = libobjgen::UUID(uuidStr.ToUtf8());
        return true;
    }
    return false;
}

bool DatabaseQuerySQLite3::GetValue(const String& name,
    libobjgen::UUID& value)
{
    size_t index;
    if(!GetResultColumnIndex(name, index))
    {
        return false;
    }

    return GetValue(index, value);
}

bool DatabaseQuerySQLite3::GetValue(size_t index, int32_t& value)
{
    if(mResultColumnTypes.size() <= index
        || mResultColumnTypes[index] != SQLITE_INTEGER)
    {
        return false;
    }

    int idx = (int)index;
    value = (int32_t)sqlite3_column_int(mStatement, idx);
    return true;
}

bool DatabaseQuerySQLite3::GetValue(const String& name, int32_t& value)
{
    size_t index;
    if(!GetResultColumnIndex(name, index))
    {
        return false;
    }

    return GetValue(index, value);
}

bool DatabaseQuerySQLite3::GetValue(size_t index, int64_t& value)
{
    if(mResultColumnTypes.size() <= index
        || mResultColumnTypes[index] != SQLITE_INTEGER)
    {
        return false;
    }

    int idx = (int)index;
    value = (int64_t)sqlite3_column_int64(mStatement, idx);
    return true;
}

bool DatabaseQuerySQLite3::GetValue(const String& name, int64_t& value)
{
    size_t index;
    if(!GetResultColumnIndex(name, index))
    {
        return false;
    }

    return GetValue(index, value);
}

bool DatabaseQuerySQLite3::GetValue(size_t index, float& value)
{
    if(mResultColumnTypes.size() <= index
        || mResultColumnTypes[index] != SQLITE_FLOAT)
    {
        return false;
    }

    int idx = (int)index;
    value = (float)sqlite3_column_double(mStatement, idx);
    return true;
}

bool DatabaseQuerySQLite3::GetValue(const String& name, float& value)
{
    size_t index = GetNamedBindingIndex(name);
    if(!GetResultColumnIndex(name, index))
    {
        return false;
    }

    return GetValue(index, value);
}

bool DatabaseQuerySQLite3::GetValue(size_t index, double& value)
{
    if(mResultColumnTypes.size() <= index
        || mResultColumnTypes[index] != SQLITE_FLOAT)
    {
        return false;
    }

    int idx = (int)index;
    value = sqlite3_column_double(mStatement, idx);
    return true;
}

bool DatabaseQuerySQLite3::GetValue(const String& name, double& value)
{
    size_t index = GetNamedBindingIndex(name);
    if(!GetResultColumnIndex(name, index))
    {
        return false;
    }

    return GetValue(index, value);
}

bool DatabaseQuerySQLite3::GetValue(size_t index, bool& value)
{
    if(mResultColumnTypes.size() <= index
        || mResultColumnTypes[index] != SQLITE_INTEGER)
    {
        return false;
    }

    int idx = (int)index;
    value = sqlite3_column_int(mStatement, idx) != 0;
    return true;
}

bool DatabaseQuerySQLite3::GetValue(const String& name, bool& value)
{
    size_t index = GetNamedBindingIndex(name);
    if(!GetResultColumnIndex(name, index))
    {
        return false;
    }

    return GetValue(index, value);
}

bool DatabaseQuerySQLite3::GetMap(size_t index,
    std::unordered_map<std::string, std::vector<char>>& values)
{
    (void)index;
    (void)values;

    /// @todo
    return false;
}

bool DatabaseQuerySQLite3::GetMap(const String& name,
    std::unordered_map<std::string, std::vector<char>>& values)
{
    (void)name;
    (void)values;

    /// @todo
    return false;
}

bool DatabaseQuerySQLite3::GetRows(std::list<std::unordered_map<
    std::string, std::vector<char>> >& rows)
{
    int colCount = sqlite3_column_count(mStatement);

    if(colCount == 0)
    {
        mStatus = SQLITE_ERROR;
        return false;
    }

    std::vector<std::string> colNames;
    std::vector<int> colTypes;
    for(int i = 0; i < colCount; i++)
    {
        colNames.push_back(std::string(sqlite3_column_name(mStatement, i)));
        colTypes.push_back(sqlite3_column_type(mStatement, i));
    }

    while(SQLITE_ROW == mStatus)
    {
        std::unordered_map<std::string, std::vector<char>> m;
        for(int i = 0; i < colCount; i++)
        {
            std::string colName = colNames[(size_t)i];
            int colType = colTypes[(size_t)i];

            std::vector<char> value;

            size_t bytes = (size_t)sqlite3_column_bytes(mStatement, i);
            switch(colType)
            {
                case SQLITE_INTEGER:
                    {
                        int val = sqlite3_column_int(mStatement, i);
                        value.insert(value.begin(),
                            reinterpret_cast<const char*>(&val),
                            reinterpret_cast<const char*>(&val) +
                            bytes);
                    }
                    break;
                case SQLITE_FLOAT:
                    {
                        double val = sqlite3_column_double(mStatement, i);
                        value.insert(value.begin(),
                            reinterpret_cast<const char*>(&val),
                            reinterpret_cast<const char*>(&val) +
                            bytes);
                    }
                    break;
                case SQLITE_BLOB:
                    {
                        auto val = sqlite3_column_blob(mStatement, i);
                        value.insert(value.begin(),
                            reinterpret_cast<const char*>(&val),
                            reinterpret_cast<const char*>(&val) +
                            bytes);
                    }
                    break;
                case SQLITE_TEXT:
                    {
                        auto val = sqlite3_column_text(mStatement, i);
                        value.insert(value.begin(), val, val + bytes);
                    }
                    break;
                case SQLITE_NULL:
                    //Nothing to add
                    break;
                default:
                    mStatus = SQLITE_ERROR;
                    break;
            }

            m[colName] = value;
        }
        rows.push_back(m);

        //Make sure the status has not been updated
        if(SQLITE_ROW == mStatus)
        {
            Next();
        }
    }

    return IsValid();
}

bool DatabaseQuerySQLite3::BatchNext()
{
    /// @todo
    return false;
}

bool DatabaseQuerySQLite3::IsValid() const
{
    return nullptr != mDatabase && nullptr != mStatement &&
        (SQLITE_OK == mStatus || SQLITE_ROW == mStatus ||
        SQLITE_DONE == mStatus);
}

int DatabaseQuerySQLite3::GetStatus() const
{
    return mStatus;
}

size_t DatabaseQuerySQLite3::GetNamedBindingIndex(const String& name) const
{
    auto binding = GetNamedBinding(name);
    return (size_t)sqlite3_bind_parameter_index(mStatement, binding.c_str());
}

std::string DatabaseQuerySQLite3::GetNamedBinding(const String& name) const
{
    return libcomp::String(":%1").Arg(name).ToUtf8();
}

bool DatabaseQuerySQLite3::GetResultColumnIndex(const String& name, size_t& index) const
{
    auto iter = std::find(mResultColumnNames.begin(), mResultColumnNames.end(), name);
    if(iter == mResultColumnNames.end())
    {
        return false;
    }

    index = (size_t)(iter - mResultColumnNames.begin());
    return true;
}
