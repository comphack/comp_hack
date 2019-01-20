@VAR_VALUE_TYPE@ @OBJECT_NAME@::Get@VAR_CAMELCASE_NAME@(@VAR_KEY_ARG_TYPE@ key)
{
    std::lock_guard<std::mutex> lock(mFieldLock);
    auto iter = @VAR_NAME@.find(key);
    if(iter != @VAR_NAME@.end())
    {
        return iter->second;
    }
    
    return @VAR_VALUE_TYPE@{};
}

bool @OBJECT_NAME@::@VAR_CAMELCASE_NAME@KeyExists(@VAR_KEY_ARG_TYPE@ key) const
{
    return @VAR_NAME@.find(key) != @VAR_NAME@.end();
}

std::list<@VAR_KEY_TYPE@> @OBJECT_NAME@::Get@VAR_CAMELCASE_NAME@Keys() const
{
    std::list<@VAR_KEY_TYPE@> results;
    for(auto& pair : @VAR_NAME@)
    {
        results.push_back(pair.first);
    }

    return results;
}

bool @OBJECT_NAME@::Set@VAR_CAMELCASE_NAME@(@VAR_KEY_ARG_TYPE@ key, @VAR_VALUE_ARG_TYPE@ val)
{
    std::lock_guard<std::mutex> lock(mFieldLock);
    if(!Validate@VAR_CAMELCASE_NAME@Entry(key, val))
    {
        return false;
    }
    
    @VAR_NAME@[key] = val;
    @PERSISTENT_CODE@
    
    return true;
}

bool @OBJECT_NAME@::Remove@VAR_CAMELCASE_NAME@(@VAR_KEY_ARG_TYPE@ key)
{
    std::lock_guard<std::mutex> lock(mFieldLock);
    auto iter = @VAR_NAME@.find(key);
    if(iter != @VAR_NAME@.end())
    {
        @VAR_NAME@.erase(key);
        @PERSISTENT_CODE@
        return true;
    }
    
    return false;
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

std::unordered_map<@VAR_KEY_TYPE@, @VAR_VALUE_TYPE@>::const_iterator @OBJECT_NAME@::@VAR_CAMELCASE_NAME@Begin() const
{
    return @VAR_NAME@.begin();
}

std::unordered_map<@VAR_KEY_TYPE@, @VAR_VALUE_TYPE@>::const_iterator @OBJECT_NAME@::@VAR_CAMELCASE_NAME@End() const
{
    return @VAR_NAME@.end();
}