([&]() -> bool
{
    for(auto& element : @VAR_NAME@)
    {
        if(!(@VAR_SAVE_CODE@))
        {
            return false;
        }
    }

    return @STREAM@.good();
})()