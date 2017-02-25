// Load a null-terminated string.
([&]() -> bool
{
    std::string s;
    char c;

    do
    {
        @STREAM@.read(&c, sizeof(c));

        if(@STREAM@.good())
        {
            s += c;
        }
    }
    while(0 != c && @STREAM@.good());

    if(!@STREAM@.good())
    {
        return false;
    }

    const char *szValue = s.c_str();

    @SET_CODE@

    return @STREAM@.good();
})()