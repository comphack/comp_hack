([&]() -> @VAR_CODE_TYPE@
{
    @VAR_CODE_TYPE@ ref;

    auto pRefChildNode = @NODE@->FirstChildElement("object");

    if(nullptr != pRefChildNode)
    {
        const char *szObjectName = pRefChildNode->Attribute("name");
        libcomp::String objectName;

        if(nullptr != szObjectName)
        {
            objectName = szObjectName;
        }

        if(objectName.IsEmpty())
        {
            ref = @CONSTRUCT_VALUE@;
        }
        else
        {
            ref = @REF_TYPE@::InheritedConstruction(objectName);
        }

        if(ref)
        {
            status = status && ref->Load(@DOC@, *pRefChildNode);
        }
        else
        {
            status = false;
        }
    }
	else
    {
        status = false;
    }

    return ref;
})()
