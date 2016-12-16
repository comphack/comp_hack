if(status)
{
    auto memberIterator = members.find(@VAR_NAME@);
    if(memberIterator != members.end())
    {
        const tinyxml2::XMLElement *@NODE@ = memberIterator->second;
        
        auto v = @ACCESS_CODE@;
        status = status && Set@VAR_CAMELCASE_NAME@(v);
    }
}