// Save a null-terminated string.
([&]() -> bool
{
    @ENCODE_CODE@
        
    if(@STREAM@.stream.good())
    {
        char zero = 0;
        @STREAM@.stream.write(&zero, sizeof(char));
    }

    return @STREAM@.stream.good();
})()