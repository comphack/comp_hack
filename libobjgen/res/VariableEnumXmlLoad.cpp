([&]() -> @VAR_CODE_TYPE@
{
    try
    {
        auto text = GetXmlText(*@NODE@);
        
        auto result = Get@VAR_NAME@Value(text, status);
        return status ? result : @VAR_CODE_TYPE@::@DEFAULT@;
    }
    catch (...)
    {
        status = false;
        return @VAR_CODE_TYPE@::@DEFAULT@;
    }
})()