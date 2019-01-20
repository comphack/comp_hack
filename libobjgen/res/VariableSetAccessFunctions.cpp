bool @OBJECT_NAME@::@VAR_CAMELCASE_NAME@Contains(@VAR_ARG_TYPE@ val)
{
    std::lock_guard<std::mutex> lock(mFieldLock);
    return @VAR_NAME@.find(val) != @VAR_NAME@.end();
}

bool @OBJECT_NAME@::Insert@VAR_CAMELCASE_NAME@(@VAR_ARG_TYPE@ val)
{
    std::lock_guard<std::mutex> lock(mFieldLock);
    if(!Validate@VAR_CAMELCASE_NAME@Entry(val))
    {
        return false;
    }

    @VAR_NAME@.insert(val);
    @PERSISTENT_CODE@

    return true;
}

bool @OBJECT_NAME@::Remove@VAR_CAMELCASE_NAME@(@VAR_ARG_TYPE@ val)
{
    std::lock_guard<std::mutex> lock(mFieldLock);

    @VAR_NAME@.erase(val);
    @PERSISTENT_CODE@

    return true;
}

void @OBJECT_NAME@::Clear@VAR_CAMELCASE_NAME@()
{
    std::lock_guard<std::mutex> lock(mFieldLock);
    @VAR_NAME@.clear();
    @PERSISTENT_CODE@
}

size_t @OBJECT_NAME@::@VAR_CAMELCASE_NAME@Count() const
{
    return @VAR_NAME@.size();
}

std::set<@VAR_TYPE@>::const_iterator @OBJECT_NAME@::@VAR_CAMELCASE_NAME@Begin() const
{
    return @VAR_NAME@.begin();
}

std::set<@VAR_TYPE@>::const_iterator @OBJECT_NAME@::@VAR_CAMELCASE_NAME@End() const
{
    return @VAR_NAME@.end();
}

std::list<@VAR_TYPE@> @OBJECT_NAME@::Get@VAR_CAMELCASE_NAME@List() const
{
    std::list<@VAR_TYPE@> results;
    for(auto entry : @VAR_NAME@)
    {
        results.push_back(entry);
    }

    return results;
}