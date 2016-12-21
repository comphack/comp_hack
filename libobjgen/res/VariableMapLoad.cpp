([&]() -> bool
{
    if(@STREAM@.dynamicSizes.empty())
    {
        return false;
    }

    uint16_t elementCount = @STREAM@.dynamicSizes.front();
    @STREAM@.dynamicSizes.pop_front();

    @VAR_NAME@.clear();
    for(uint16_t i = 0; i < elementCount; ++i)
    {
        @VAR_KEY_TYPE@ keyElem;
        {
            @VAR_KEY_TYPE@ element;

            if(!(@VAR_KEY_LOAD_CODE@))
            {
                return false;
            }
            
            keyElem = element;
        }
        
        @VAR_VALUE_TYPE@ valueElem;
        {
            @VAR_VALUE_TYPE@ element;

            if(!(@VAR_VALUE_LOAD_CODE@))
            {
                return false;
            }
            
            valueElem = element;
        }

        @VAR_NAME@[keyElem] = valueElem;
    }

    return @STREAM@.stream.good();
})()