([&]() -> bool
{
    for(auto& element : @VAR_NAME@)
    {
        if(!(@VAR_SAVE_CODE@))
        {
            return false;
        }
    }

    @STREAM@.dynamicSizes.push_back(static_cast<uint16_t>(
        @VAR_NAME@.size()));

    return @STREAM@.stream.good();
})()