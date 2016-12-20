([&]() -> @VAR_CODE_TYPE@
{
    if(!@VAR_NAME@.IsNull())
    {
        @VAR_NAME@.Get()->Load(@DOC@, *@NODE@);
    }

    return @VAR_NAME@;
})()
