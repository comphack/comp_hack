([&]() -> @VAR_CODE_TYPE@
{
    @VAR_CODE_TYPE@ ref = @CONSTRUCT_VALUE@;
    ref.Get()->Load(@DOC@, *@NODE@);

    return ref;
})()
