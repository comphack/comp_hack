([&]() -> @VAR_CODE_TYPE@
{
    @VAR_CODE_TYPE@ ref = @CONSTRUCT_VALUE@;
    ref->Load(@DOC@, *@NODE@);

    return ref;
})()
