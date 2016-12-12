([&]() -> bool
{
    for(auto kv : @VAR_NAME@)
    {
        {
            auto value = kv.first;
            if(!(@VAR_KEY_VALID_CODE@))
            {
                return false;
            }
        }
        
        {
            auto value = kv.second;
            if(!(@VAR_VALUE_VALID_CODE@))
            {
                return false;
            }
        }
    }

    return true;
})()