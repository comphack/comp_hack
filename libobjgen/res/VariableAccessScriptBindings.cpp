namespace libcomp
{
    template<>
    ScriptEngine& ScriptEngine::Using<@OBJECT_NAME@>()
    {
        Sqrat::Class<@OBJECT_NAME@> binding(mVM, @OBJECT_STRING_NAME@);
        binding
            @BINDINGS@; // Last call to binding

        Sqrat::RootTable(mVM).Bind(@OBJECT_STRING_NAME@, binding);
        
        return *this;
    }
}