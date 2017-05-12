([&]()
{
    if(mDirtyFields.find(@COLUMN_NAME@) != mDirtyFields.end())
    {
        return true;
    }

    libobjgen::UUID value;

    if(!query.GetValue(@COLUMN_NAME@, value))
    {
        return false;
    }
    
    return @VAR_NAME@.SetUUID(value);
}())