([&]() -> bool
{
    if(flat)
    {
        std::vector<char> data;
        data.reserve(sizeof(uint64_t) * 2);

        bool good = @STREAM@.read(&data[0], static_cast<std::streamsize>(
            data.size())).good();

        if(good)
        {
            auto uuid = libobjgen::UUID(data);
            good = @VAR_NAME@.SetUUID(uuid);
        }

        return good;
    }
	
	return true;
})()
