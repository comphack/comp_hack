namespace libcomp
{
    @DEPENDENCY_PROTOTYPES@
    template<>
    ScriptEngine& ScriptEngine::Using<@OBJECT_NAME@>()
    {
        if(!BindingExists(@OBJECT_STRING_NAME@, true))
        {
            @PARENT_DEPENDENCY@
            Sqrat::@BINDING_TYPE@ binding(mVM, @OBJECT_STRING_NAME@);
            Bind<@OBJECT_NAME@>(@OBJECT_STRING_NAME@, binding);

            @DEPENDENCIES@
            @ADDITIONS@
            @BINDINGS@
        }

        return *this;
    }
}
