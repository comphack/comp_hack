([&]()
{
    @DATABASE_TYPE@ value;

    return query.GetValue(@COLUMN_NAME@, value) &&
        @SET_FUNCTION@(static_cast<@VAR_TYPE@>(value));
}())
