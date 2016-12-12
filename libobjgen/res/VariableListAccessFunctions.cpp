@VAR_TYPE@ @OBJECT_NAME@::Get@VAR_CAMELCASE_NAME@(size_t index) const
{
    if(@VAR_NAME@.size() <= index)
    {
        return @VAR_TYPE@{};
    }
    
    auto it = @VAR_NAME@.begin();
    std::advance(it, index);
    return *it;
}

void @OBJECT_NAME@::Append@VAR_CAMELCASE_NAME@(@VAR_TYPE@ val)
{
    @VAR_NAME@.push_back(val);
}

void @OBJECT_NAME@::Prepend@VAR_CAMELCASE_NAME@(@VAR_TYPE@ val)
{
    @VAR_NAME@.push_front(val);
}

bool @OBJECT_NAME@::Insert@VAR_CAMELCASE_NAME@(size_t index, @VAR_TYPE@ val)
{
    if(@VAR_NAME@.size() <= index)
    {
        return false;
    }
    
    auto it = @VAR_NAME@.begin();
    std::advance(it, index);
    @VAR_NAME@.insert(it, val);
    
    return true;
}

bool @OBJECT_NAME@::Remove@VAR_CAMELCASE_NAME@(size_t index)
{
    if(@VAR_NAME@.size() <= index)
    {
        return false;
    }
    
    auto it = @VAR_NAME@.begin();
    std::advance(it, index);
    @VAR_NAME@.erase(it);
    
    return true;
}

void @OBJECT_NAME@::Clear@VAR_CAMELCASE_NAME@()
{
    @VAR_NAME@.clear();
}

std::list<@VAR_TYPE@>::iterator @OBJECT_NAME@::@VAR_CAMELCASE_NAME@Begin()
{
    return @VAR_NAME@.begin();
}

std::list<@VAR_TYPE@>::iterator @OBJECT_NAME@::@VAR_CAMELCASE_NAME@End()
{
    return @VAR_NAME@.end();
}