([&]()
{
    if(mDirtyFields.find(@COLUMN_NAME@) != mDirtyFields.end())
    {
        return true;
    }

    return query.GetValue(@COLUMN_NAME@, @VAR_NAME@);
}())