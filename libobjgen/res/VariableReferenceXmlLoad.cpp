([&]() -> @VAR_CODE_TYPE@
{
    @VAR_CODE_TYPE@ ref = nullptr;

    auto pRefChildNode = @NODE@->FirstChildElement("object");

    if(nullptr != pRefChildNode)
    {
        @CONSTRUCT_CODE@

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
        // Parent node exists but is empty. Leave as null but don't error.
    }

    return ref;
})()
