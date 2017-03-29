{
    tinyxml2::XMLText *pText = @DOC@.NewText(@GETTER@.C());
    pText->SetCData(true);

    tinyxml2::XMLElement *pMember = @DOC@.NewElement(@ELEMENT_NAME@);
    if(!std::string(@VAR_NAME@).empty()) pMember->SetAttribute("name", @VAR_NAME@);
    pMember->InsertEndChild(pText);

    @PARENT@->InsertEndChild(pMember);
}