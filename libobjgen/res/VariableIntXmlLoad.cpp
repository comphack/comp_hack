([&]() -> @VAR_CODE_TYPE@
{
    try
    {
        return libcomp::String(GetXmlText(*@NODE@)).ToInteger<@VAR_CODE_TYPE@>();
    }
    catch (...)
    {
        status = false;
        return @VAR_CODE_TYPE@{};
    }
})()