std::string @OBJECT_NAME@::Get@VAR_CAMELCASE_NAME@String(@VAR_TYPE@ val) const
{
    switch(val)
    {
        @CASES@
        default:
            break;
    }
    
    return "";
}

@VAR_TYPE@ @OBJECT_NAME@::Get@VAR_CAMELCASE_NAME@Value(std::string& val, bool& success)
{
    success = true;
    
    @CONDITIONS@
    
    success = false;
    return (@VAR_TYPE@)0;
}