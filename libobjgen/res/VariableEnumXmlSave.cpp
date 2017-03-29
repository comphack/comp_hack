{
    tinyxml2::XMLElement *pMember = doc.NewElement(@ELEMENT_NAME@);
    if(!std::string(@VAR_XML_NAME@).empty()) pMember->SetAttribute("name", @VAR_XML_NAME@);

    tinyxml2::XMLText *pText = doc.NewText(Get@VAR_CAMELCASE_NAME@String(@GETTER@).c_str());
    pMember->InsertEndChild(pText);

    @PARENT@->InsertEndChild(pMember);
}