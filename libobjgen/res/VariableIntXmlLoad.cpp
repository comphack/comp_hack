if (status)
{
    std::unordered_map<std::string,
        const tinyxml2::XMLElement*>::const_iterator memberIterator =
        @MEMBERS@.find("@VAR_NAME@");

    if(memberIterator != @MEMBERS@.end())
    {
        const tinyxml2::XMLElement *pMember = memberIterator->second;

        try
        {
            double val = std::stod(GetXmlText(*pMember));
            if (!Set@VAR_CAMELCASE_NAME@((@VAR_CODE_TYPE@)val))
            {
                status = false;
            }
        }
        catch(...)
        {
            status = false;
        }
    }
}