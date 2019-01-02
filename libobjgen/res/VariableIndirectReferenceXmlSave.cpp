{
    tinyxml2::XMLElement *pMember = doc.NewElement(@ELEMENT_NAME@);
    if(!std::string(@VAR_XML_NAME@).empty()) pMember->SetAttribute("name", @VAR_XML_NAME@);

    if(!@VAR_NAME@.IsNull())
    {
        tinyxml2::XMLText *pText = doc.NewText(@VAR_NAME@.ToString().c_str());
        pMember->InsertEndChild(pText);
    }

    @PARENT@->InsertEndChild(pMember);
}