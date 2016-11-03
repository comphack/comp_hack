([&]() -> bool
{
    uint32_t elementCount = static_cast<uint32_t>(@VAR_NAME@.size());

    @STREAM@.write(reinterperet_cast<char*>(&elementCount),
        sizeof(elementCount));

    if(!@STREAM@.good())
    {
        return false;
    }

    for(auto& element : @VAR_NAME@)
    {
        if(!(@VAR_SAVE_CODE@))
        {
            return false;
        }
    }

    return @STREAM@.good();
})()