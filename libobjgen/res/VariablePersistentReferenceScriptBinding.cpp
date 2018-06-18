Sqrat::Class<libcomp::ObjectReference<@OBJECT_NAME@>> refBinding(mVM, "@OBJECT_NAME@Ref");
refBinding
    .Func("IsNull", &libcomp::ObjectReference<@OBJECT_NAME@>::IsNull)
    .Func("GetUUID", &libcomp::ObjectReference<@OBJECT_NAME@>::GetUUID)
    .Func("SetUUID", &libcomp::ObjectReference<@OBJECT_NAME@>::SetUUID)
    .StaticFunc("Unload", &libcomp::ObjectReference<@OBJECT_NAME@>::Unload)
    .Func("GetCurrentReference", &libcomp::ObjectReference<
        @OBJECT_NAME@>::GetCurrentReference)
    .Func<void (libcomp::ObjectReference<@OBJECT_NAME@>::*)(
        const std::shared_ptr<@OBJECT_NAME@>&)>("SetReference",
        &libcomp::ObjectReference<@OBJECT_NAME@>::SetReference)
    .Overload<const std::shared_ptr<@OBJECT_NAME@>
        (libcomp::ObjectReference<@OBJECT_NAME@>::*)()>(
        "Get", &libcomp::ObjectReference<@OBJECT_NAME@>::Get)
    .Overload<const std::shared_ptr<@OBJECT_NAME@>
        (libcomp::ObjectReference<@OBJECT_NAME@>::*)(const std::shared_ptr<
        libcomp::Database>&, bool)>("Get",
        &libcomp::ObjectReference<@OBJECT_NAME@>::Get)
    ; // Last call to binding

Bind<libcomp::ObjectReference<@OBJECT_NAME@>>("@OBJECT_NAME@Ref", refBinding);
