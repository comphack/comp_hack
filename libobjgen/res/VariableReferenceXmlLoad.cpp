([&]() -> @VAR_CODE_TYPE@
{
    @VAR_CODE_TYPE@ ref(std::shared_ptr<@REF_TYPE@>(new @REF_TYPE@));
    ref.Get()->Load(@DOC@, *@NODE@);

    return ref;
})()
