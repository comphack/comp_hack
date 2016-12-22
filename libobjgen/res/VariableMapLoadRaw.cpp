([&]() -> bool
{
    uint32_t elementCount = 0;

    @STREAM@.read(reinterpret_cast<char*>(&elementCount),
        sizeof(elementCount));

    if(!@STREAM@.good())
    {
        return false;
    }

    @VAR_NAME@.clear();
    for(uint32_t i = 0; i < elementCount; ++i)
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

    return @STREAM@.good();
})()