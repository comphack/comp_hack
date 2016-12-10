([&]() -> bool
{
    uint32_t elementCount = 0;

    @STREAM@.read(reinterpret_cast<char*>(&elementCount),
        sizeof(elementCount));

    if(!@STREAM@.good())
    {
        return false;
    }

    for(uint32_t i = 0; i < elementCount; ++i)
    {
        @VAR_TYPE@ element;

        if(!(@VAR_LOAD_CODE@))
        {
            return false;
        }

        @VAR_NAME@.push_back(element);
    }

    return @STREAM@.good();
})()