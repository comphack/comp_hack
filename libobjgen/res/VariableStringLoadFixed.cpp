// Load a string with a fixed size buffer.
([&]() -> bool
{
    char szValue[@FIXED_LENGTH@];
    szValue[sizeof(szValue) - 1] = 0;

    @STREAM@.read(szValue, sizeof(szValue) - 1);

    if(!@STREAM@.good())
    {
        return false;
    }

    @SET_CODE@

    return @STREAM@.good();
})()