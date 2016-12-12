([&]() -> bool
{
    uint32_t elementCount = static_cast<uint32_t>(@VAR_NAME@.size());

    @STREAM@.write(reinterpret_cast<char*>(&elementCount),
        sizeof(elementCount));

    if(!@STREAM@.good())
    {
        return false;
    }

    for(auto& kv : @VAR_NAME@)
    {
        {
            auto element = kv.first;
            if(!(@VAR_KEY_SAVE_CODE@))
            {
                return false;
            }
        }
        
        {
            auto element = kv.second;
            if(!(@VAR_VALUE_SAVE_CODE@))
            {
                return false;
            }
        }
    }

    return @STREAM@.good();
})()