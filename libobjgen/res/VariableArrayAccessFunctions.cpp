@VAR_TYPE@ @OBJECT_NAME@::Get@VAR_CAMELCASE_NAME@(size_t index) const
{
    if(@ELEMENT_COUNT@ <= index)
    {
        return @VAR_TYPE@{};
    }

    return @VAR_NAME@[index];
}

bool @OBJECT_NAME@::Set@VAR_CAMELCASE_NAME@(size_t index, @VAR_ARG_TYPE@ val)
{
    if(@ELEMENT_COUNT@ <= index || !Validate@VAR_CAMELCASE_NAME@Entry(val))
    {
        return false;
    }

    @VAR_NAME@[index] = val;
    
    return true;
}

size_t @OBJECT_NAME@::@VAR_CAMELCASE_NAME@Count() const
{
    return @VAR_NAME@.size();
}