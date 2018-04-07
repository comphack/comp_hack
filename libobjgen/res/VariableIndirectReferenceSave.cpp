([&]() -> bool
{
    std::vector<char> data = @VAR_NAME@.ToData();

    return @STREAM@.stream.write(&data[0], static_cast<std::streamsize>(
        data.size())).good();
})()