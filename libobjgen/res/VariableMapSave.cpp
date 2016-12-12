([&]() -> bool
{
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

    @STREAM@.dynamicSizes.push_back(static_cast<uint16_t>(
        @VAR_NAME@.size()));

    return @STREAM@.stream.good();
})()