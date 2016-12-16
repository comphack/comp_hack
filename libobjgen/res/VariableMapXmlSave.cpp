{
    tinyxml2::XMLElement *temp = @PARENT@;
    {
        tinyxml2::XMLElement *pMember = doc.NewElement(@ELEMENT_NAME@);
        pMember->SetAttribute("name", @VAR_NAME@);
        
        @PARENT@->InsertEndChild(pMember);
        
        @PARENT@ = pMember;
    }

    for(auto kv : @GETTER@)
    {
        {
            auto element = kv.first;
            @VAR_XML_KEY_SAVE_CODE@
        }
        
        {
            auto element = kv.second;
            @VAR_XML_VALUE_SAVE_CODE@
        }
    }

    @PARENT@ = temp;
}