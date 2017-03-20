([&]() -> @VAR_CODE_TYPE@
{
    @VAR_CODE_TYPE@ arr = @DEFAULT_VALUE@;
    
    auto elements = GetXmlChildren(*@NODE@, "element");
    if(elements.size() <= @ELEMENT_COUNT@)
    {
        auto elemIter = elements.begin();
        for(size_t i = 0; i < @ELEMENT_COUNT@; i++)
        {
            if(i < elements.size())
            {
                auto element = *elemIter;
                elemIter++;
                arr[i] = @ELEMENT_ACCESS_CODE@;
            }
        }
    }
    else
    {
        status = false;
    }
    
    return arr;
})()