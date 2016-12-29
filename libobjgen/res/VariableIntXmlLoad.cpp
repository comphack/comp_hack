([&]() -> @VAR_CODE_TYPE@
{
    try
    {
        auto retval = libcomp::String(GetXmlText(*@NODE@)).@CONVERT_FUNC@<@VAR_CODE_TYPE@>(&status);
        return status ? retval : @VAR_CODE_TYPE@{};
    }
    catch (...)
    {
        status = false;
        return @VAR_CODE_TYPE@{};
    }
})()
