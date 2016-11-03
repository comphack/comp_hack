([&]() -> bool
{
    for(auto& value : @VAR_NAME@)
    {
        if(!(@VAR_SAVE_CODE@))
        {
            return false;
        }
    }

    return @STREAM@.good();
})()