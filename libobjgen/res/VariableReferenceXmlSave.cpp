if(nullptr != @VAR_NAME@)
{
    tinyxml2::XMLElement *temp = @PARENT@;
    {
        tinyxml2::XMLElement *pMember = doc.NewElement(@ELEMENT_NAME@);
        pMember->SetAttribute("name", @VAR_XML_NAME@);
        
        @PARENT@->InsertEndChild(pMember);
        
        @PARENT@ = pMember;
    }

    @VAR_NAME@->Save(@DOC@, *@PARENT@, false);

    @PARENT@ = temp;
}