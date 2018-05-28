([&]() -> @VAR_CODE_TYPE@
{
    @VAR_CODE_TYPE@ arr = @DEFAULT_VALUE@;
    
    auto elements = GetXmlChildren(*@NODE@, "element");
    if(elements.size() <= @ELEMENT_COUNT@)
    {
        auto elemIter = elements.begin();
        for(size_t i = 0; i < @ELEMENT_COUNT@ && elemIter != elements.end(); i++)
        {
            auto element = *elemIter;
            auto attr = element->Attribute("index");
            if(attr)
            {
                // Skip forward to specified index
                size_t idx = libcomp::String(attr).ToInteger<size_t>(&status);
                if(idx >= i)
                {
                    i = idx;
                }
                else
                {
                    status = false;
                    break;
                }
            }

            if(i < @ELEMENT_COUNT@)
            {
                elemIter++;
                arr[i] = @ELEMENT_ACCESS_CODE@;
            }
            else
            {
                status = false;
                break;
            }
        }
    }
    else
    {
        status = false;
    }
    
    return arr;
})()