([&]() -> bool
{
    auto uidSize = sizeof(uint64_t) * 2;
    char* buffer = new char[uidSize];

    bool good = @STREAM@.stream.read(buffer, static_cast<std::streamsize>(
        uidSize)).good();

    std::vector<char> data;
    data.insert(data.begin(), buffer, buffer + uidSize);

    if(good)
    {
        auto uuid = libobjgen::UUID(data);
        good = @VAR_NAME@.SetUUID(uuid);
    }

    delete[] buffer;

    return good;
})()