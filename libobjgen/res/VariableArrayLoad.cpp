([&]() -> bool
{
    @PERSIST_COPY@
    for(auto& element : @VAR_NAME@)
    {
        if(!(@VAR_LOAD_CODE@))
        {
            return false;
        }
    }

    return @STREAM@.good();
})()