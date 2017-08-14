([&]() -> @VAR_CODE_TYPE@
{
    @VAR_CODE_TYPE@ s;
    
    for(auto element : GetXmlChildren(*@NODE@, "element"))
    {
        auto elem = @ELEMENT_ACCESS_CODE@;
        if(status)
        {
            s.insert(elem);
        }
    }
    
    return s;
})()