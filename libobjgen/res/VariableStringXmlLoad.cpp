if(status)
{
    std::unordered_map<std::string,
        const tinyxml2::XMLElement*>::const_iterator memberIterator =
            @MEMBERS@.find("@VAR_NAME@");

    if(memberIterator != @MEMBERS@.end())
    {
        const tinyxml2::XMLElement *pMember = memberIterator->second;

        if(!Set@VAR_CAMELCASE_NAME@(GetXmlText(*pMember)))
        {
            status = false;
        }
    }
}