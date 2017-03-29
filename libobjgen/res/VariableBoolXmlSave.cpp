{
    tinyxml2::XMLElement *pMember = doc.NewElement(@ELEMENT_NAME@);
    if(!std::string(@VAR_NAME@).empty()) pMember->SetAttribute("name", @VAR_NAME@);

    tinyxml2::XMLText *pText = doc.NewText(@GETTER@ ? "true" : "false");
    pMember->InsertEndChild(pText);

    @PARENT@->InsertEndChild(pMember);
}