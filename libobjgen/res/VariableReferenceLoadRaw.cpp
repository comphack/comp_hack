[&]()
{
    if(flat)
    {
        std::vector<char> data;
        data.reserve(sizeof(uint64_t) * 2);

        bool good = @STREAM@.read(&data[0], data.size()).good();

        if(good)
        {
            mUUID = libobjgen::UUID(data);
        }

        return good;
    }
    else
    {
        return @VAR_NAME@ && @VAR_NAME@->Load(@STREAM@);
    }
}