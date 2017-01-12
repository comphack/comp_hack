([&]() -> bool
{
    libobjgen::UUID uuid;

    if(!@VAR_NAME@.IsNull())
    {
        uuid = @VAR_NAME@.GetUUID();
    }

    std::vector<char> data = uuid.ToData();

    return @STREAM@.write(&data[0], static_cast<std::streamsize>(
        data.size())).good();
})()
