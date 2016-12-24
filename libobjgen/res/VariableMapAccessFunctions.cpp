@VAR_VALUE_TYPE@ @OBJECT_NAME@::Get@VAR_CAMELCASE_NAME@(@VAR_KEY_ARG_TYPE@ key) const
{
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

bool @OBJECT_NAME@::Set@VAR_CAMELCASE_NAME@(@VAR_KEY_ARG_TYPE@ key, @VAR_VALUE_ARG_TYPE@ val)
{
    if(!Validate@VAR_CAMELCASE_NAME@Entry(key, val))
    {
        return false;
    }
    
    @VAR_NAME@[key] = val;
    
    return true;
}

bool @OBJECT_NAME@::Remove@VAR_CAMELCASE_NAME@(@VAR_KEY_ARG_TYPE@ key)
{
    auto iter = @VAR_NAME@.find(key);
    if(iter != @VAR_NAME@.end())
    {
        @VAR_NAME@.erase(key);
        return true;
    }
    
    return false;
}

void @OBJECT_NAME@::Clear@VAR_CAMELCASE_NAME@()
{
    @VAR_NAME@.clear();
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