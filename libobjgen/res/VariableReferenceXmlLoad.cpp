([&]() -> @VAR_CODE_TYPE@
{
    @VAR_CODE_TYPE@ ref;

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
        status = false;
    }

    return ref;
})()
