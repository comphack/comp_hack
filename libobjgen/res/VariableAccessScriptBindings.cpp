namespace libcomp
{
    template<>
    ScriptEngine& ScriptEngine::Using<@OBJECT_NAME@>()
    {
        if(!BindingExists(@OBJECT_STRING_NAME@, true))
        {
            @DEPENDENCIES@
            Sqrat::@BINDING_TYPE@ binding(mVM, @OBJECT_STRING_NAME@);
            @BINDINGS@

            Bind<@OBJECT_NAME@>(@OBJECT_STRING_NAME@, binding);

            @ADDITIONS@
        }
        
        return *this;
    }
}