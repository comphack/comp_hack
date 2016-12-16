([&]() -> @VAR_CODE_TYPE@
{
    try
    {
        auto retval = libcomp::String(GetXmlText(*@NODE@)).ToInteger<@VAR_CODE_TYPE@>(&status);
        return status ? retval : @VAR_CODE_TYPE@{};
    }
    catch (...)
    {
        status = false;
        return @VAR_CODE_TYPE@{};
    }
})()