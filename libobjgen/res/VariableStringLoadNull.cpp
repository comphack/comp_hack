// Load a null-terminated string.
([&]() -> bool
{
    std::string s;
    char c;

    do
    {
        @STREAM@.stream.read(&c, sizeof(c));

        if(@STREAM@.stream.good())
        {
            s += c;
        }
    }
    while(0 != c && @STREAM@.stream.good());

    if(!@STREAM@.stream.good())
    {
        return false;
    }

    const char *szValue = s.c_str();

    @SET_CODE@

    return @STREAM@.stream.good();
})()