([&]()
{
    if(mDirtyFields.find(@COLUMN_NAME@) != mDirtyFields.end())
    {
        return true;
    }

    @DATABASE_TYPE@ value;

    if(!query.GetValue(@COLUMN_NAME@, value))
    {
        return false;
    }
    else
    {
        @VAR_NAME@ = value;
        return true;
    }
}())
