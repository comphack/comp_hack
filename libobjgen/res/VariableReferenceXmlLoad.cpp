([&]() -> @VAR_CODE_TYPE@
{
    @VAR_CODE_TYPE@ ref = @CONSTRUCT_VALUE@;

    auto pRefChildNode = @NODE@->FirstChildElement("object");

    /// @todo This does not handle errors... at all.
    if(nullptr != pRefChildNode)
    {
        /// @todo Check if the load failed!
        (void)ref->Load(@DOC@, *pRefChildNode);
    }

    return ref;
})()
