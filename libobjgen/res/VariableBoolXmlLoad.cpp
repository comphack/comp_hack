([&]() -> bool
{
    try
    {
        return GetXmlText(*@NODE@) == "true";
    }
    catch (...)
    {
        status = false;
        return false;
    }
})()