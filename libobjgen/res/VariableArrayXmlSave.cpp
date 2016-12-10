tinyxml2::XMLElement *temp@VAR_NAME@ = @PARENT@;
{
    tinyxml2::XMLElement *pMember = doc.NewElement("@ELEMENT_NAME@");
    pMember->SetAttribute("name", "@VAR_NAME@");
	
    @PARENT@->InsertEndChild(pMember);
	
	@PARENT@ = pMember;
}

for(auto element : @GETTER@)
{
	@VAR_XML_SAVE_CODE@
}

@PARENT@ = temp@VAR_NAME@;