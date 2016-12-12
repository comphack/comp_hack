([&]() -> @VAR_CODE_TYPE@
{
    @VAR_CODE_TYPE@ l;
    
    for(auto element : GetXmlChildren(*@NODE@, "element"))
    {
        auto elem = @ELEMENT_ACCESS_CODE@;
        if(status)
        {
            l.push_back(elem);
        }
    }
    
    return l;
})()