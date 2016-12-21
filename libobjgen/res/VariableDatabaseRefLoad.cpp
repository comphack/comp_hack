([&]()
{
    UUID value;

    if(!query.GetValue(@COLUMN_NAME@, value))
    {
        return false;
    }
    
    return @VAR_NAME@.SetUUID(value);
}())