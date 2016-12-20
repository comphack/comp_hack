([&]() -> @VAR_CODE_TYPE@
{
	if(!@VAR_NAME@.IsNull())
    {
        auto uuid = GetXmlText(*@NODE@);
        @VAR_NAME@.SetUUID(uuid);
    }

    return @VAR_NAME@;
})()
