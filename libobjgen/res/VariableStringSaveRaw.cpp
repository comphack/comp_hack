// Load a string with a size specified.
([&]() -> bool
{
    std::vector<char> value = libcomp::Convert::ToEncoding(
        @ENCODING@, @VAR_NAME@);

    @LENGTH_TYPE@ len = static_cast<@LENGTH_TYPE@>(value.size());

    @STREAM@.write(reinterpret_cast<char*>(&len),
        sizeof(len));

    if(!@STREAM@.good())
    {
        return false;
    }

    if(!value.empty())
    {
        @STREAM@.write(&value[0], static_cast<std::streamsize>(
            value.size()));
    }

    return @STREAM@.good();
})()