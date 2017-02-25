// Save a null-terminated string.
([&]() -> bool
{
    @ENCODE_CODE@
        
    if(@STREAM@.good())
    {
        char zero = 0;
        @STREAM@.write(&zero, sizeof(char));
    }

    return @STREAM@.good();
})()