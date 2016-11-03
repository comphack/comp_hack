// Load a string with a size specified.
([&]() -> bool
{
    std::string value = @VAR_NAME@.ToUtf8();

    @LENGTH_TYPE@ len = static_cast<@LENGTH_TYPE@>(value.size());

    @STREAM@.write(reinterpret_cast<char*>(&len),
        sizeof(len));

    if(!@STREAM@.good())
    {
        return false;
    }

    if(!value.empty())
    {
        @STREAM@.write(value.c_str(), static_cast<std::streamsize>(
            value.size()));
    }

    return @STREAM@.good();
})()