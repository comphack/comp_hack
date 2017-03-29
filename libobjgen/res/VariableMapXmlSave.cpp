{
    tinyxml2::XMLElement *pTemp = @PARENT@;
    tinyxml2::XMLElement *pMapMember = nullptr;

    pMapMember = doc.NewElement(@ELEMENT_NAME@);
    pMapMember->SetAttribute("name", @VAR_NAME@);

    @PARENT@->InsertEndChild(pMapMember);

    for(auto kv : @GETTER@)
    {
        @PARENT@ = doc.NewElement("pair");

        pMapMember->InsertEndChild(@PARENT@);

        {
            auto element = kv.first;
            @VAR_XML_KEY_SAVE_CODE@
        }

        {
            auto element = kv.second;
            @VAR_XML_VALUE_SAVE_CODE@
        }
    }

    @PARENT@ = pTemp;
}
