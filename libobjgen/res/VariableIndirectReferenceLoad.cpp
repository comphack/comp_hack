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
        @VAR_NAME@ = libobjgen::UUID(data);
    }

    delete[] buffer;

    return good;
})()