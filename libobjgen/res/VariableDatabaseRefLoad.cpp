([&]()
{
    UUID value;

    if(!query.GetValue(@COLUMN_NAME@, value))
    {
        return false;
    }

    std::shared_ptr<@VAR_TYPE@> obj = PersistentObject::LoadObjectByUUID<
        @VAR_TYPE@>(value);

    return nullptr != obj && @SET_FUNCTION@(obj);
}())