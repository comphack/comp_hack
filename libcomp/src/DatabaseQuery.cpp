/**
 * @file libcomp/src/DatabaseQuery.cpp
 * @ingroup libcomp
 *
 * @author COMP Omega <compomega@tutanota.com>
 *
 * @brief Base class to handle a database query.
 *
 * This file is part of the COMP_hack Library (libcomp).
 *
 * Copyright (C) 2012-2018 COMP_hack Team <compomega@tutanota.com>
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

#include "DatabaseQuery.h"

using namespace libcomp;

DatabaseQueryImpl::DatabaseQueryImpl() : mAffectedRowCount(0)
{
}

DatabaseQueryImpl::~DatabaseQueryImpl()
{
}

bool DatabaseQueryImpl::Bind(size_t index, const std::unordered_map<
    std::string, std::vector<char>>& values)
{
    (void)index;
    (void)values;

    return false;
}

bool DatabaseQueryImpl::Bind(const String& name, const std::unordered_map<
    std::string, std::vector<char>>& values)
{
    (void)name;
    (void)values;

    return false;
}

bool DatabaseQueryImpl::GetRows(std::list<std::unordered_map<
    std::string, std::vector<char>>>& rows)
{
    (void)rows;

    return false;
}

int64_t DatabaseQueryImpl::AffectedRowCount() const
{
    return mAffectedRowCount;
}

DatabaseQuery::DatabaseQuery(DatabaseQueryImpl *pImpl) : mImpl(pImpl)
{
}

DatabaseQuery::DatabaseQuery(DatabaseQueryImpl *pImpl, const String& query) :
    mImpl(pImpl)
{
    Prepare(query);
}

DatabaseQuery::DatabaseQuery(DatabaseQuery&& other) : mImpl(other.mImpl)
{
    other.mImpl = nullptr;
}

DatabaseQuery::~DatabaseQuery()
{
    delete mImpl;
    mImpl = nullptr;
}

bool DatabaseQuery::Prepare(const String& query)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->Prepare(query);
    }

    return result;
}

bool DatabaseQuery::Execute()
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->Execute();
    }

    return result;
}

bool DatabaseQuery::Next()
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->Next();
    }

    return result;
}

bool DatabaseQuery::Bind(size_t index, const String& value)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->Bind(index, value);
    }

    return result;
}

bool DatabaseQuery::Bind(const String& name, const String& value)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->Bind(name, value);
    }

    return result;
}

bool DatabaseQuery::Bind(size_t index, const std::vector<char>& value)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->Bind(index, value);
    }

    return result;
}

bool DatabaseQuery::Bind(const String& name, const std::vector<char>& value)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->Bind(name, value);
    }

    return result;
}

bool DatabaseQuery::Bind(size_t index, const libobjgen::UUID& value)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->Bind(index, value);
    }

    return result;
}

bool DatabaseQuery::Bind(const String& name, const libobjgen::UUID& value)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->Bind(name, value);
    }

    return result;
}

bool DatabaseQuery::Bind(size_t index, int32_t value)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->Bind(index, value);
    }

    return result;
}

bool DatabaseQuery::Bind(const String& name, int32_t value)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->Bind(name, value);
    }

    return result;
}

bool DatabaseQuery::Bind(size_t index, int64_t value)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->Bind(index, value);
    }

    return result;
}

bool DatabaseQuery::Bind(const String& name, int64_t value)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->Bind(name, value);
    }

    return result;
}

bool DatabaseQuery::Bind(size_t index, float value)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->Bind(index, value);
    }

    return result;
}

bool DatabaseQuery::Bind(const String& name, float value)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->Bind(name, value);
    }

    return result;
}

bool DatabaseQuery::Bind(size_t index, double value)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->Bind(index, value);
    }

    return result;
}

bool DatabaseQuery::Bind(const String& name, double value)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->Bind(name, value);
    }

    return result;
}

bool DatabaseQuery::Bind(size_t index, bool value)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->Bind(index, value);
    }

    return result;
}

bool DatabaseQuery::Bind(const String& name, bool value)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->Bind(name, value);
    }

    return result;
}

bool DatabaseQuery::IsValid() const
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->IsValid();
    }

    return result;
}

bool DatabaseQuery::Bind(size_t index, const std::unordered_map<
    std::string, std::vector<char>>& values)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->Bind(index, values);
    }

    return result;
}

bool DatabaseQuery::Bind(const String& name, const std::unordered_map<
    std::string, std::vector<char>>& values)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->Bind(name, values);
    }

    return result;
}

bool DatabaseQuery::GetValue(size_t index, String& value)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->GetValue(index, value);
    }

    return result;
}

bool DatabaseQuery::GetValue(const String& name, String& value)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->GetValue(name, value);
    }

    return result;
}

bool DatabaseQuery::GetValue(size_t index, std::vector<char>& value)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->GetValue(index, value);
    }

    return result;
}

bool DatabaseQuery::GetValue(const String& name, std::vector<char>& value)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->GetValue(name, value);
    }

    return result;
}

bool DatabaseQuery::GetValue(size_t index, libobjgen::UUID& value)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->GetValue(index, value);
    }

    return result;
}

bool DatabaseQuery::GetValue(const String& name, libobjgen::UUID& value)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->GetValue(name, value);
    }

    return result;
}

bool DatabaseQuery::GetValue(size_t index, int32_t& value)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->GetValue(index, value);
    }

    return result;
}

bool DatabaseQuery::GetValue(const String& name, int32_t& value)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->GetValue(name, value);
    }

    return result;
}

bool DatabaseQuery::GetValue(size_t index, int64_t& value)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->GetValue(index, value);
    }

    return result;
}

bool DatabaseQuery::GetValue(const String& name, int64_t& value)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->GetValue(name, value);
    }

    return result;
}

bool DatabaseQuery::GetValue(size_t index, float& value)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->GetValue(index, value);
    }

    return result;
}

bool DatabaseQuery::GetValue(const String& name, float& value)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->GetValue(name, value);
    }

    return result;
}

bool DatabaseQuery::GetValue(size_t index, double& value)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->GetValue(index, value);
    }

    return result;
}

bool DatabaseQuery::GetValue(const String& name, double& value)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->GetValue(name, value);
    }

    return result;
}

bool DatabaseQuery::GetValue(size_t index, bool& value)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->GetValue(index, value);
    }

    return result;
}

bool DatabaseQuery::GetValue(const String& name, bool& value)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->GetValue(name, value);
    }

    return result;
}

bool DatabaseQuery::GetRows(std::list<std::unordered_map<
    std::string, std::vector<char>>>& rows)
{
    bool result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->GetRows(rows);
    }

    return result;
}

int64_t DatabaseQuery::AffectedRowCount() const
{
    int64_t result = false;

    if(nullptr != mImpl)
    {
        result = mImpl->AffectedRowCount();
    }

    return result;
}

DatabaseQuery& DatabaseQuery::operator=(DatabaseQuery&& other)
{
    delete mImpl;

    mImpl = other.mImpl;
    other.mImpl = nullptr;

    return *this;
}
