// Save a string with a fixed size buffer.
([&]() -> bool
{
    static const char zero[@FIXED_LENGTH@] = { 0 };

    @ENCODE_CODE@

    if(@STREAM@.good() && @FIXED_LENGTH@ != value.size())
    {
        @STREAM@.write(zero, static_cast<std::streamsize>(
            @FIXED_LENGTH@) - static_cast<std::streamsize>(value.size()));
    }

    return @STREAM@.good();
})()