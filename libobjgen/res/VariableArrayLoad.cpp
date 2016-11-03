([&]() -> bool
{
    for(auto& value : @VAR_NAME@)
    {
        if(!(@VAR_LOAD_CODE@))
        {
            return false;
        }
    }

    return @STREAM@.good();
})()