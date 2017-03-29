([&]() -> @VAR_CODE_TYPE@
{
    @VAR_CODE_TYPE@ ref;

    auto pRefChildNode = @NODE@->FirstChildElement("object");

    /// @todo This does not handle errors... at all.
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

        /// @todo Check if the load failed!
        if(ref)
        {
            (void)ref->Load(@DOC@, *pRefChildNode);
        }
    }

    return ref;
})()
