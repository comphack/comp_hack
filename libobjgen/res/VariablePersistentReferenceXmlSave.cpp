if(!@VAR_NAME@.IsNull())
{
    tinyxml2::XMLElement *pMember = doc.NewElement(@ELEMENT_NAME@);
    pMember->SetAttribute("name", @VAR_XML_NAME@);

    tinyxml2::XMLText *pText = doc.NewText(@VAR_NAME@.GetCurrentReference()->GetUUID()
        .ToString().c_str());
    pMember->InsertEndChild(pText);

    @PARENT@->InsertEndChild(pMember);
}