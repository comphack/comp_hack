([&]() -> @VAR_CODE_TYPE@
{
    @VAR_CODE_TYPE@ m;
    
    auto pairs = GetXmlChildren(*@NODE@, "pair");
    for(auto pair : pairs)
    {
        auto @KEY_NODE@ = GetXmlChild(*pair, "key");
        auto @VALUE_NODE@ = GetXmlChild(*pair, "value");
        
        if(nullptr != @KEY_NODE@ && nullptr != @VALUE_NODE@)
        {
            auto key = @KEY_ACCESS_CODE@;
            auto val = @VALUE_ACCESS_CODE@;
            
            if(m.find(key) == m.end())
            {
                m[key] = val;
            }
            else
            {
                status = false;
            }
        }
        else
        {
            status = false;
        }
    }
    
    return m;
})()