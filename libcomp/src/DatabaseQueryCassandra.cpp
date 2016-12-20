/**
 * @file libcomp/src/DatabaseQueryCassandra.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief A Cassandra database query.
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

#include "DatabaseQueryCassandra.h"
#include "DatabaseCassandra.h"

using namespace libcomp;

DatabaseQueryCassandra::DatabaseQueryCassandra(DatabaseCassandra *pDatabase) :
    mDatabase(pDatabase), mPrepared(nullptr), mStatement(nullptr),
    mFuture(nullptr), mResult(nullptr), mRowIterator(nullptr), mBatch(nullptr)
{
}

DatabaseQueryCassandra::~DatabaseQueryCassandra()
{
    if(nullptr != mBatch)
    {
        cass_batch_free(mBatch);
        mBatch = nullptr;
    }

    if(nullptr != mRowIterator)
    {
        cass_iterator_free(mRowIterator);
        mRowIterator = 0;
    }

    if(nullptr != mResult)
    {
        cass_result_free(mResult);
        mResult = nullptr;
    }

    if(nullptr != mFuture)
    {
        cass_future_free(mFuture);
        mFuture = nullptr;
    }

    if(nullptr != mStatement)
    {
        cass_statement_free(mStatement);
        mStatement = nullptr;
    }

    if(nullptr != mPrepared)
    {
        cass_prepared_free(mPrepared);
        mPrepared = nullptr;
    }
}

bool DatabaseQueryCassandra::Prepare(const String& query)
{
    bool result = true;

    // Remove any existing (prepared) statement.
    if(nullptr != mStatement)
    {
        cass_statement_free(mStatement);
        mStatement = nullptr;
    }

    if(nullptr != mPrepared)
    {
        cass_prepared_free(mPrepared);
        mPrepared = nullptr;
    }

    CassFuture *pFuture = cass_session_prepare(mDatabase->GetSession(),
        query.C());

    cass_future_wait(pFuture);

    if(CASS_OK != cass_future_error_code(pFuture))
    {
        result = mDatabase->WaitForFuture(pFuture);
    }
    else
    {
        mPrepared = cass_future_get_prepared(pFuture);

        cass_future_free(pFuture);
        pFuture = nullptr;

        mStatement = cass_prepared_bind(mPrepared);

        if(nullptr == mStatement)
        {
            cass_prepared_free(mPrepared);
            mPrepared = nullptr;

            result = false;
        }
    }

    return result;
}

bool DatabaseQueryCassandra::Execute()
{
    bool result = false;

    if(nullptr != mFuture)
    {
        cass_iterator_free(mRowIterator);
        mRowIterator = nullptr;

        cass_result_free(mResult);
        mResult = nullptr;

        cass_future_free(mFuture);
        mFuture = nullptr;
    }

    if(nullptr != mStatement && nullptr != mDatabase)
    {
        CassSession *pSession = mDatabase->GetSession();

        if(nullptr != pSession)
        {
            CassFuture *pFuture;

            if(nullptr != mBatch)
            {
                if(CASS_OK == cass_batch_add_statement(mBatch, mStatement))
                {
                    cass_statement_free(mStatement);
                    mStatement = nullptr;

                    pFuture = cass_session_execute_batch(pSession, mBatch);
                }
                else
                {
                    pFuture = nullptr;
                }
            }
            else
            {
                pFuture = cass_session_execute(pSession, mStatement);
            }

            if(nullptr != pFuture)
            {
                cass_future_wait(pFuture);
            }

            if(nullptr == pFuture || CASS_OK != cass_future_error_code(
                pFuture))
            {
                if(nullptr != pFuture)
                {
                    result = mDatabase->WaitForFuture(pFuture);
                }
                else
                {
                    result = false;
                }
            }
            else
            {
                // Free the batch.
                if(nullptr != mBatch)
                {
                    cass_batch_free(mBatch);
                    mBatch = nullptr;
                }

                // Free the statement.
                if(nullptr != mStatement)
                {
                    cass_statement_free(mStatement);
                    mStatement = nullptr;
                }

                // Prepare another statement.
                if(nullptr != mPrepared)
                {
                    mStatement = cass_prepared_bind(mPrepared);
                }

                // Save the result.
                mResult = cass_future_get_result(pFuture);

                // Save a row iterator.
                if(nullptr != mResult)
                {
                    mRowIterator = cass_iterator_from_result(mResult);
                }

                // Save the future.
                mFuture = pFuture;

                result = true;
            }
        }
    }

    return result;
}

bool DatabaseQueryCassandra::Next()
{
    bool result = false;

    if(nullptr != mRowIterator)
    {
        result = cass_iterator_next(mRowIterator) ? true : false;
    }

    return result;
}

bool DatabaseQueryCassandra::Bind(size_t index, const String& value)
{
    bool result = false;

    if(nullptr != mStatement)
    {
        result = CASS_OK == cass_statement_bind_string_n(mStatement,
            index, value.C(), value.Size());
    }

    return result;
}

bool DatabaseQueryCassandra::Bind(const String& name, const String& value)
{
    bool result = false;

    if(nullptr != mStatement)
    {
        result = CASS_OK == cass_statement_bind_string_by_name_n(mStatement,
            name.C(), name.Size(), value.C(), value.Size());
    }

    return result;
}

bool DatabaseQueryCassandra::Bind(size_t index, const std::vector<char>& value)
{
    bool result = false;

    if(nullptr != mStatement)
    {
        result = CASS_OK == cass_statement_bind_bytes(mStatement, index,
            reinterpret_cast<const cass_byte_t*>(&value[0]), value.size());
    }

    return result;
}

bool DatabaseQueryCassandra::Bind(const String& name,
    const std::vector<char>& value)
{
    bool result = false;

    if(nullptr != mStatement)
    {
        result = CASS_OK == cass_statement_bind_bytes_by_name_n(mStatement,
            name.C(), name.Size(), reinterpret_cast<const cass_byte_t*>(
                &value[0]), value.size());
    }

    return result;
}

bool DatabaseQueryCassandra::Bind(size_t index, const libobjgen::UUID& value)
{
    bool result = false;

    if(nullptr != mStatement)
    {
        result = CASS_OK == cass_statement_bind_uuid(mStatement, index,
            value.ToCassandra());
    }

    return result;
}

bool DatabaseQueryCassandra::Bind(const String& name,
    const libobjgen::UUID& value)
{
    bool result = false;

    if(nullptr != mStatement)
    {
        result = CASS_OK == cass_statement_bind_uuid_by_name_n(mStatement,
            name.C(), name.Size(), value.ToCassandra());
    }

    return result;
}

bool DatabaseQueryCassandra::Bind(size_t index, int32_t value)
{
    bool result = false;

    if(nullptr != mStatement)
    {
        result = CASS_OK == cass_statement_bind_int32(mStatement, index, value);
    }

    return result;
}

bool DatabaseQueryCassandra::Bind(const String& name, int32_t value)
{
    bool result = false;

    if(nullptr != mStatement)
    {
        result = CASS_OK == cass_statement_bind_int32_by_name_n(mStatement,
            name.C(), name.Size(), value);
    }

    return result;
}

bool DatabaseQueryCassandra::Bind(size_t index, int64_t value)
{
    bool result = false;

    if(nullptr != mStatement)
    {
        result = CASS_OK == cass_statement_bind_int64(mStatement, index, value);
    }

    return result;
}

bool DatabaseQueryCassandra::Bind(const String& name, int64_t value)
{
    bool result = false;

    if(nullptr != mStatement)
    {
        result = CASS_OK == cass_statement_bind_int64_by_name_n(mStatement,
            name.C(), name.Size(), value);
    }

    return result;
}

bool DatabaseQueryCassandra::Bind(size_t index, float value)
{
    bool result = false;

    if(nullptr != mStatement)
    {
        result = CASS_OK == cass_statement_bind_float(mStatement, index, value);
    }

    return result;
}

bool DatabaseQueryCassandra::Bind(const String& name, float value)
{
    bool result = false;

    if(nullptr != mStatement)
    {
        result = CASS_OK == cass_statement_bind_float_by_name_n(mStatement,
            name.C(), name.Size(), value);
    }

    return result;
}

bool DatabaseQueryCassandra::Bind(size_t index, double value)
{
    bool result = false;

    if(nullptr != mStatement)
    {
        result = CASS_OK == cass_statement_bind_double(mStatement, index,
            value);
    }

    return result;
}

bool DatabaseQueryCassandra::Bind(const String& name, double value)
{
    bool result = false;

    if(nullptr != mStatement)
    {
        result = CASS_OK == cass_statement_bind_double_by_name_n(mStatement,
            name.C(), name.Size(), value);
    }

    return result;
}

bool DatabaseQueryCassandra::Bind(size_t index, bool value)
{
    bool result = false;

    if(nullptr != mStatement)
    {
        result = CASS_OK == cass_statement_bind_bool(mStatement, index,
            value ? cass_true : cass_false);
    }

    return result;
}

bool DatabaseQueryCassandra::Bind(const String& name, bool value)
{
    bool result = false;

    if(nullptr != mStatement)
    {
        result = CASS_OK == cass_statement_bind_bool_by_name_n(mStatement,
            name.C(), name.Size(), value ? cass_true : cass_false);
    }

    return result;
}

bool DatabaseQueryCassandra::Bind(size_t index, const std::unordered_map<
    std::string, std::vector<char>>& values)
{
    bool result = false;

    if(nullptr != mStatement)
    {
        CassCollection *pCollection = cass_collection_new(
            CASS_COLLECTION_TYPE_MAP, values.size());

        if(nullptr != pCollection)
        {
            result = true;

            for(auto it : values)
            {
                const std::string& name = it.first;
                const std::vector<char>& value = it.second;

                if(result && CASS_OK == cass_collection_append_string_n(
                    pCollection, name.c_str(), name.size()))
                {
                    if(CASS_OK == cass_collection_append_bytes(
                        pCollection, reinterpret_cast<const cass_byte_t*>(
                            &value[0]), value.size()))
                    {
                        //
                    }
                    else
                    {
                        result = false;
                    }
                    
                    result = CASS_OK == cass_statement_bind_collection(
                        mStatement, index, pCollection);
                }
                else
                {
                    result = false;
                }
            } // for(auto it : values)

            cass_collection_free(pCollection);
        } // if(nullptr != pCollection)
    } // if(nullptr != mStatement)

    return result;
}

bool DatabaseQueryCassandra::Bind(const String& name, const std::unordered_map<
    std::string, std::vector<char>>& values)
{
    bool result = false;

    if(nullptr != mStatement)
    {
        CassCollection *pCollection = cass_collection_new(
            CASS_COLLECTION_TYPE_MAP, values.size());

        if(nullptr != pCollection)
        {
            result = true;

            for(auto it : values)
            {
                const std::string& columnName = it.first;
                const std::vector<char>& value = it.second;

                if(result && CASS_OK == cass_collection_append_string_n(
                    pCollection, columnName.c_str(), columnName.size()))
                {
                    if(CASS_OK == cass_collection_append_bytes(
                        pCollection, reinterpret_cast<const cass_byte_t*>(
                            &value[0]), value.size()))
                    {
                        //
                    }
                    else
                    {
                        result = false;
                    }
                    
                    if(CASS_OK != cass_statement_bind_collection_by_name_n(
                        mStatement, name.C(), name.Size(), pCollection))
                    {
                        result = false;
                    }
                }
                else
                {
                    result = false;
                }
            } // for(auto it : values)

            cass_collection_free(pCollection);
        } // if(nullptr != pCollection)
    } // if(nullptr != mStatement)

    return result;
}

const CassValue* DatabaseQueryCassandra::GetValue(size_t index)
{
    const CassValue *pValue = nullptr;
    const CassRow *pRow;

    if(nullptr != mRowIterator && nullptr != (pRow = cass_iterator_get_row(
        mRowIterator)))
    {
        pValue = cass_row_get_column(pRow, index);
    }

    return pValue;
}

const CassValue* DatabaseQueryCassandra::GetValue(const String& name)
{
    const CassValue *pValue = nullptr;
    const CassRow *pRow;

    if(nullptr != mRowIterator && nullptr != (pRow = cass_iterator_get_row(
        mRowIterator)))
    {
        pValue = cass_row_get_column_by_name_n(pRow,
            name.C(), name.Size());
    }

    return pValue;
}

bool DatabaseQueryCassandra::GetTextValue(const CassValue *pValue,
    String& value)
{
    bool result = false;

    if(nullptr != pValue)
    {
        const char *szValueData = nullptr;
        size_t valueSize = 0;

        if(CASS_OK == cass_value_get_string(pValue, &szValueData,
            &valueSize) && nullptr != szValueData && 0 != valueSize)
        {
            value = String(szValueData, valueSize);

            result = true;
        }
    }

    return result;
}

bool DatabaseQueryCassandra::GetValue(size_t index, String& value)
{
    return GetTextValue(GetValue(index), value);
}

bool DatabaseQueryCassandra::GetValue(const String& name, String& value)
{
    return GetTextValue(GetValue(name), value);
}

bool DatabaseQueryCassandra::GetBlobValue(const CassValue *pValue,
    std::vector<char>& value)
{
    bool result = false;

    if(nullptr != pValue)
    {
        const cass_byte_t *pValueData = nullptr;
        size_t valueSize = 0;

        if(CASS_OK == cass_value_get_bytes(pValue, &pValueData,
            &valueSize) && nullptr != pValueData && 0 != valueSize)
        {
            value.clear();
            value.insert(value.begin(),
                reinterpret_cast<const char*>(pValueData),
                reinterpret_cast<const char*>(pValueData + valueSize));

            result = true;
        }
    }

    return result;
}

bool DatabaseQueryCassandra::GetValue(size_t index, std::vector<char>& value)
{
    return GetBlobValue(GetValue(index), value);
}

bool DatabaseQueryCassandra::GetValue(const String& name,
    std::vector<char>& value)
{
    return GetBlobValue(GetValue(name), value);
}

bool DatabaseQueryCassandra::GetUuidValue(const CassValue *pValue,
    libobjgen::UUID& value)
{
    bool result = false;

    if(nullptr != pValue)
    {
        CassUuid uuid;

        if(CASS_OK == cass_value_get_uuid(pValue, &uuid))
        {
            value = libobjgen::UUID(uuid);

            result = true;
        }
    }

    return result;
}

bool DatabaseQueryCassandra::GetValue(size_t index, libobjgen::UUID& value)
{
    return GetUuidValue(GetValue(index), value);
}

bool DatabaseQueryCassandra::GetValue(const String& name,
    libobjgen::UUID& value)
{
    return GetUuidValue(GetValue(name), value);
}

bool DatabaseQueryCassandra::GetIntValue(const CassValue *pValue,
    int32_t& value)
{
    bool result = false;

    if(nullptr != pValue)
    {
        if(CASS_OK == cass_value_get_int32(pValue, &value))
        {
            result = true;
        }
    }

    return result;
}

bool DatabaseQueryCassandra::GetValue(size_t index, int32_t& value)
{
    return GetIntValue(GetValue(index), value);
}

bool DatabaseQueryCassandra::GetValue(const String& name, int32_t& value)
{
    return GetIntValue(GetValue(name), value);
}

bool DatabaseQueryCassandra::GetBigIntValue(const CassValue *pValue,
    int64_t& value)
{
    bool result = false;

    if(nullptr != pValue)
    {
        if(CASS_OK == cass_value_get_int64(pValue, &value))
        {
            result = true;
        }
    }

    return result;
}

bool DatabaseQueryCassandra::GetValue(size_t index, int64_t& value)
{
    return GetBigIntValue(GetValue(index), value);
}

bool DatabaseQueryCassandra::GetValue(const String& name, int64_t& value)
{
    return GetBigIntValue(GetValue(name), value);
}

bool DatabaseQueryCassandra::GetFloatValue(const CassValue *pValue,
    float& value)
{
    bool result = false;

    if(nullptr != pValue)
    {
        if(CASS_OK == cass_value_get_float(pValue, &value))
        {
            result = true;
        }
    }

    return result;
}

bool DatabaseQueryCassandra::GetValue(size_t index, float& value)
{
    return GetFloatValue(GetValue(index), value);
}

bool DatabaseQueryCassandra::GetValue(const String& name, float& value)
{
    return GetFloatValue(GetValue(name), value);
}

bool DatabaseQueryCassandra::GetDoubleValue(const CassValue *pValue,
    double& value)
{
    bool result = false;

    if(nullptr != pValue)
    {
        if(CASS_OK == cass_value_get_double(pValue, &value))
        {
            result = true;
        }
    }

    return result;
}

bool DatabaseQueryCassandra::GetValue(size_t index, double& value)
{
    return GetDoubleValue(GetValue(index), value);
}

bool DatabaseQueryCassandra::GetValue(const String& name, double& value)
{
    return GetDoubleValue(GetValue(name), value);
}

bool DatabaseQueryCassandra::GetBoolValue(const CassValue *pValue,
    bool& value)
{
    bool result = false;

    if(nullptr != pValue)
    {
        cass_bool_t val;

        if(CASS_OK == cass_value_get_bool(pValue, &val))
        {
            value = (cass_true == val);

            result = true;
        }
    }

    return result;
}

bool DatabaseQueryCassandra::GetValue(size_t index, bool& value)
{
    return GetBoolValue(GetValue(index), value);
}

bool DatabaseQueryCassandra::GetValue(const String& name, bool& value)
{
    return GetBoolValue(GetValue(name), value);
}


bool DatabaseQueryCassandra::GetMap(size_t index,
    std::unordered_map<std::string, std::vector<char>>& values)
{
    bool result = false;

    const CassRow *pRow;

    if(nullptr != mRowIterator && nullptr != (pRow = cass_iterator_get_row(
        mRowIterator)))
    {
        const CassValue *pColumn = cass_row_get_column(pRow, index);

        CassIterator *pMapIterator;

        if(nullptr != pColumn)
        {
            pMapIterator = cass_iterator_from_map(pColumn);

            if(nullptr != pMapIterator)
            {
                result = true;

                while(result && cass_iterator_next(pMapIterator))
                {
                    const CassValue *pKey = cass_iterator_get_map_key(
                        pMapIterator);
                    const CassValue *pValue = cass_iterator_get_map_value(
                            pMapIterator);

                    std::string key;
                    const char *szKey;
                    size_t keySize;

                    if(nullptr != pKey && CASS_OK == cass_value_get_string(
                        pKey, &szKey, &keySize))
                    {
                        key = std::string(szKey, keySize);
                    }
                    else
                    {
                        result = false;
                    }

                    std::vector<char> value;
                    const cass_byte_t *pValueData;
                    size_t valueSize;

                    if(result && nullptr != pKey && CASS_OK ==
                        cass_value_get_bytes(pValue, &pValueData, &valueSize))
                    {
                        value.insert(value.begin(),
                            reinterpret_cast<const char*>(pValueData),
                            reinterpret_cast<const char*>(pValueData) +
                            valueSize);
                    }
                    else
                    {
                        result = false;
                    }

                    if(result)
                    {
                        values[key] = std::move(value);
                    }
                }

                cass_iterator_free(pMapIterator);
                pMapIterator = nullptr;
            }
        }
    }

    return result;
}

bool DatabaseQueryCassandra::GetMap(const String& name,
    std::unordered_map<std::string, std::vector<char>>& values)
{
    bool result = false;

    const CassRow *pRow;

    if(nullptr != mRowIterator && nullptr != (pRow = cass_iterator_get_row(
        mRowIterator)))
    {
        const CassValue *pColumn = cass_row_get_column_by_name_n(pRow,
            name.C(), name.Size());

        CassIterator *pMapIterator;

        if(nullptr != pColumn)
        {
            pMapIterator = cass_iterator_from_map(pColumn);

            if(nullptr != pMapIterator)
            {
                result = true;

                while(result && cass_iterator_next(pMapIterator))
                {
                    const CassValue *pKey = cass_iterator_get_map_key(
                        pMapIterator);
                    const CassValue *pValue = cass_iterator_get_map_value(
                            pMapIterator);

                    std::string key;
                    const char *szKey;
                    size_t keySize;

                    if(nullptr != pKey && CASS_OK == cass_value_get_string(
                        pKey, &szKey, &keySize))
                    {
                        key = std::string(szKey, keySize);
                    }
                    else
                    {
                        result = false;
                    }

                    std::vector<char> value;
                    const cass_byte_t *pValueData;
                    size_t valueSize;

                    if(result && nullptr != pKey && CASS_OK ==
                        cass_value_get_bytes(pValue, &pValueData, &valueSize))
                    {
                        value.insert(value.begin(),
                            reinterpret_cast<const char*>(pValueData),
                            reinterpret_cast<const char*>(pValueData) +
                            valueSize);
                    }
                    else
                    {
                        result = false;
                    }

                    if(result)
                    {
                        values[key] = std::move(value);
                    }
                }

                cass_iterator_free(pMapIterator);
                pMapIterator = nullptr;
            }
        }
    }

    return result;
}

bool DatabaseQueryCassandra::GetRows(std::list<std::unordered_map<
    std::string, std::vector<char>> >& rows)
{
    bool result = true;

    if(nullptr != mResult)
    {
        size_t colCount = cass_result_column_count(mResult);

        std::vector<std::string> colNames;
        for(size_t i = 0; i < colCount; i++)
        {
            const char* colName;
            size_t nameLength;
            cass_result_column_name(mResult, i, &colName, &nameLength);
            colNames.push_back(std::string(colName, nameLength));
        }

        CassIterator* rowIter = cass_iterator_from_result(mResult);

        while(cass_iterator_next(rowIter))
        {
            const CassRow* pRow = cass_iterator_get_row(rowIter);
            std::unordered_map<std::string, std::vector<char>> m;
            for(size_t k = 0; k < colCount; k++)
            {
                const CassValue* pValue = cass_row_get_column(pRow, k);

                const cass_byte_t *pValueData;
                size_t valueSize;

                if(CASS_OK == cass_value_get_bytes(pValue, &pValueData, &valueSize))
                {
                    std::vector<char> value;
                    value.insert(value.begin(),
                        reinterpret_cast<const char*>(pValueData),
                        reinterpret_cast<const char*>(pValueData) +
                        valueSize);

                    auto colName = colNames[k];
                    m[colName] = value;
                }
                else
                {
                    result = false;
                }
            }
            rows.push_back(m);
        }
    }

    return result;
}

bool DatabaseQueryCassandra::BatchNext()
{
    bool result = false;

    if(nullptr != mStatement)
    {
        if(nullptr == mBatch)
        {
            mBatch = cass_batch_new(CASS_BATCH_TYPE_LOGGED);
        }

        if(nullptr != mBatch)
        {
            if(CASS_OK == cass_batch_add_statement(mBatch, mStatement))
            {
                cass_statement_free(mStatement);
                mStatement = nullptr;

                if(nullptr != mPrepared)
                {
                    mStatement = cass_prepared_bind(mPrepared);
                }

                result = true;
            }
        }
    }

    return result;
}

bool DatabaseQueryCassandra::IsValid() const
{
    return nullptr != mDatabase && nullptr != mPrepared &&
        nullptr != mStatement;
}
