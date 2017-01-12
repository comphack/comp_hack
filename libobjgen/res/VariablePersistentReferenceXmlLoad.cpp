([&]() -> @VAR_CODE_TYPE@
{
    @VAR_CODE_TYPE@ ref;
    auto uuid = GetXmlText(*@NODE@);
    ref.SetUUID(uuid);

    return ref;
})()
